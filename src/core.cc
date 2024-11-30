#include "core.h"
#include "fmt/core.h"
#include "music_list.h"
#include "runtime.h"
#include <cmath>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>

SoundPack::SoundPack()
    : sound_init_notification_(new SoundInitNotification)
{
    this->sound_init_notification_->cb_.onSignal = soundInitNotificationCallback;
    this->sound_init_notification_->data_        = this;
}
SoundPack::~SoundPack()
{
    this->uninit();
}

ma_result SoundPack::init(ma_engine* engine, std::string_view file_path, ma_uint32 flags)
{
    std::lock_guard<std::mutex> lock(this->mtx_);
    if (this->initialized_) {
        log("can not init sound twice", LogType::ERROR, __func__);
        return MA_ERROR;
    }
    if (!this->sound_) { this->sound_ = std::unique_ptr<ma_sound>(new ma_sound); }
    ma_sound_config config                      = ma_sound_config_init_2(engine);
    config.pFilePath                            = file_path.data();
    config.flags                                = flags;
    config.initNotifications.done.pNotification = this->sound_init_notification_.get();

    return ma_sound_init_ex(engine, &config, this->sound_.get());
}

void SoundPack::uninit()
{
    if (this->initialized_ && this->sound_) {
        std::lock_guard<std::mutex> lock(this->mtx_);
        ma_sound_uninit(this->sound_.get());
        this->initialized_ = false;
        this->playing_     = false;
        log("uninit sound", LogType::INFO, __func__);
    }
}

bool SoundPack::isInitialized()
{
    return this->initialized_;
}
bool SoundPack::isPlaying()
{
    return this->playing_;
}

MaComponents::MaComponents()
    : engine_(new ma_engine)
    , sound_to_play_(new SoundPack)
    , sound_to_register_(new SoundPack)
{
}

MaComponents::~MaComponents()
{
    ma_engine_uninit(engine_.get());
}

void MaComponents::maCompInitEngine()
{
    ma_result result = ma_engine_init(nullptr, engine_.get());
    if (result != MA_SUCCESS) {
        log("failed to initialize engine", LogType::ERROR, __func__);
        throw std::runtime_error("failed to initialize engine");
    }
}

UserData::UserData(
    ma_engine* engine,
    SoundPack* sound_to_play,
    SoundPack* sound_to_register,
    QuitController* quit_controller,
    MusicList* music_list,
    std::string* music_playing,
    bool* random,
    bool* repetitive
)
    : engine_(engine)
    , sound_to_play_(sound_to_play)
    , sound_to_register_(sound_to_register)
    , quit_controller_(quit_controller)
    , music_list_(music_list)
    , music_playing_(music_playing)
    , random_(random)
    , repetitive_(repetitive)
{
}

void QuitController::signal()
{
    std::lock_guard<std::mutex> lock(this->mtx_);
    this->end_ = true;
    this->cv_.notify_one();
}

void QuitController::reset()
{
    this->end_ = false;
}
void QuitController::quit()
{
    std::lock_guard<std::mutex> lock(this->mtx_);
    this->quit_flag = true;
    this->cv_.notify_one();
}

bool QuitController::atEnd()
{
    return this->end_;
}
bool QuitController::getQuitFlag()
{
    return this->quit_flag;
}

ma_result playInternal(
    ma_engine* engine,
    SoundPack* sound_to_play,
    std::string_view music_to_play,
    std::string* music_playing,
    MusicList* music_list,
    SoundPack* sound_to_register,
    QuitController* quit_controller,
    bool* random,
    bool* repetitive
)
{
    *music_playing = music_to_play;

    {
        std::lock_guard<std::mutex> lock(sound_to_register->mtx_);
        sound_to_register->playing_ = false;
    }

    ma_result result = sound_to_play->init(engine, music_to_play, LOADING_FLAGS);
    if (result != MA_SUCCESS) {
        log(fmt::format("failed to initialize sound from file: {}", music_to_play), LogType::ERROR, __func__);
        return result;
    }

    if (*random) {
        auto music = music_list->randomOut();
        if (!music) {
            log(fmt::format("failed to get random music: {}", music_to_play), LogType::ERROR, __func__);
            return MA_ERROR;
        }
        music_list->headIn(music->getMusic());
    }

    auto* user_data(new UserData(
        engine,
        sound_to_register,
        sound_to_play,
        quit_controller,
        music_list,
        music_playing,
        random,
        repetitive
    ));

    ma_sound_set_end_callback(sound_to_play->sound_.get(), soundAtEndCallback, user_data);
    result = ma_sound_start(sound_to_play->sound_.get());
    if (result != MA_SUCCESS) {
        log(fmt::format("failed to start sound: {}", music_to_play), LogType::ERROR, __func__);
        return result;
    }
    {
        std::lock_guard<std::mutex> lock(sound_to_play->mtx_);
        sound_to_play->playing_ = true;
    }
    log(fmt::format("started playing: {}", music_to_play), LogType::INFO, __func__);
    return MA_SUCCESS;
}

void cleanFunc(MaComponents* ma_comp, QuitController* quit_controller)
{
    std::unique_lock<std::mutex> lock(quit_controller->mtx_);
    while (true) {
        quit_controller->cv_.wait(lock, [quit_controller] {
            return quit_controller->atEnd() || quit_controller->getQuitFlag();
        });
        if (quit_controller->getQuitFlag()) {
            log("cleaner quit", LogType::INFO, __func__);
            return;
        }
        if (!ma_comp->sound_to_play_->isPlaying() && ma_comp->sound_to_play_->isInitialized()) {
            ma_comp->sound_to_play_->uninit();
        }
        if (!ma_comp->sound_to_register_->isPlaying() && ma_comp->sound_to_register_->isInitialized()) {
            ma_comp->sound_to_register_->uninit();
        }
        quit_controller->reset();
    }
}

void soundAtEndCallback(void* user_data, ma_sound* sound)
{
    auto* data = static_cast<UserData*>(user_data);
    if (!data) {
        log("got an invalid user_data", LogType::ERROR, __func__);
        throw std::runtime_error("got an invalid user_data");
    }
    const std::string& music_playing = *(data->music_playing_);

    if (!*(data->repetitive_)) { data->music_list_->remove(music_playing); }
    data->music_list_->tailIn(music_playing);

    const std::string MUSIC_TO_PLAY = data->music_list_->headOut()->getMusic();

    ma_result result = playInternal(
        data->engine_,
        data->sound_to_play_,
        MUSIC_TO_PLAY,
        data->music_playing_,
        data->music_list_,
        data->sound_to_register_,
        data->quit_controller_,
        data->random_,
        data->repetitive_
    );
    if (result != MA_SUCCESS) {
        log(fmt::format("failed to play music: {}", MUSIC_TO_PLAY), LogType::ERROR, __func__);
        throw std::runtime_error("failed to play music");
    }
    data->quit_controller_->signal();
    delete data;
}

void soundInitNotificationCallback(ma_async_notification* notification)
{
    auto* sound_init_notification = static_cast<SoundInitNotification*>(notification);
    auto* data                    = static_cast<SoundPack*>(sound_init_notification->data_);
    if (!data) {
        log("got an invalid data", LogType::ERROR, __func__);
        throw std::runtime_error("got an invalid data");
    }
    data->initialized_ = true;
    log("sound initialized", LogType::INFO, __func__);
}