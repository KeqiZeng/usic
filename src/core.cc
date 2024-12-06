#include "core.h"
#include "fmt/core.h"
#include "music_list.h"
#include "runtime.h"
#include "utils.h"
#include <atomic>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>

SoundPack::SoundPack()
    : sound_init_notification_(std::make_unique<SoundInitNotification>())
{
    sound_init_notification_->cb_.onSignal = soundInitNotificationCallback;
    sound_init_notification_->data_        = this;
}
SoundPack::~SoundPack()
{
    uninit();
}

ma_result SoundPack::init(ma_engine* engine, std::string_view file_path, ma_uint32 flags)
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (initialized_.load(std::memory_order_acquire)) {
        LOG("cannot initialize twice", LogType::ERROR);
        return MA_ERROR;
    }

    sound_                                      = std::make_unique<ma_sound>();
    ma_sound_config config                      = ma_sound_config_init_2(engine);
    config.pFilePath                            = file_path.data();
    config.flags                                = flags;
    config.initNotifications.done.pNotification = sound_init_notification_.get();

    ma_result result = ma_sound_init_ex(engine, &config, sound_.get());
    if (result != MA_SUCCESS) {
        LOG("failed to initialize sound", LogType::ERROR);
        sound_.reset();
        initialized_.store(false, std::memory_order_release);
        return result;
    }

    return MA_SUCCESS;
}

void SoundPack::uninit()
{
    std::lock_guard<std::mutex> lock(mtx_);

    if (!initialized_.load(std::memory_order_acquire)) { return; }

    if (!sound_) { return; }

    ma_result result = ma_sound_stop(sound_.get());
    if (result != MA_SUCCESS) { LOG("failed to stop sound", LogType::ERROR); }

    ma_sound_uninit(sound_.get());

    initialized_.store(false, std::memory_order_release);
    playing_.store(false, std::memory_order_release);

    sound_.reset();
    LOG("uninitialized sound", LogType::INFO);
}

bool SoundPack::isInitialized()
{
    return initialized_.load(std::memory_order_acquire);
}
bool SoundPack::isPlaying()
{
    return playing_.load(std::memory_order_acquire);
}

MaComponents::MaComponents()
    : engine_(std::make_unique<ma_engine>())
    , sound_to_play_(std::make_unique<SoundPack>())
    , sound_to_register_(std::make_unique<SoundPack>())
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
        LOG("failed to initialize engine", LogType::ERROR);
        throw std::runtime_error("failed to initialize engine");
    }
}

UserData::UserData(
    ma_engine* engine,
    SoundPack* sound_to_play,
    SoundPack* sound_to_register,
    Controller* controller,
    MusicList* music_list,
    std::string* music_playing,
    Config* config
)
    : engine_(engine)
    , sound_to_play_(sound_to_play)
    , sound_to_register_(sound_to_register)
    , controller_(controller)
    , music_list_(music_list)
    , music_playing_(music_playing)
    , config_(config)
{
}

void Controller::signalEnd()
{
    end_.store(true, std::memory_order_release);
    flag_.test_and_set(std::memory_order_release);
    flag_.notify_one();
}

void Controller::signalError()
{
    error_.store(true, std::memory_order_release);
    flag_.test_and_set(std::memory_order_release);
    flag_.notify_one();
}

void Controller::quit()
{
    quit_flag_.store(true, std::memory_order_release);
    flag_.test_and_set(std::memory_order_release);
    flag_.notify_one();
}

bool Controller::atEnd()
{
    return end_.load(std::memory_order_acquire);
}

bool Controller::getError()
{
    return error_.load(std::memory_order_acquire);
}

bool Controller::getQuitFlag()
{
    return quit_flag_.load(std::memory_order_acquire);
}

void Controller::waitForCleanerExit()
{
    while (!cleaner_exited_.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
}

ma_result playInternal(
    ma_engine* engine,
    SoundPack* sound_to_play,
    std::string_view music_to_play,
    std::string* music_playing,
    MusicList* music_list,
    SoundPack* sound_to_register,
    Controller* controller,
    Config* config
)
{
    *music_playing = music_to_play;

    sound_to_register->playing_.store(false, std::memory_order_release);

    ma_result result =
        sound_to_play->init(engine, fmt::format("{}{}", config->getUsicLibrary(), music_to_play), LOADING_FLAGS);
    if (result != MA_SUCCESS) {
        LOG(fmt::format("failed to initialize sound from file: {}", music_to_play), LogType::ERROR);
        return result;
    }

    if (config->isRandom()) {
        auto music = music_list->randomOut();
        if (!music) {
            LOG(fmt::format("failed to get random music: {}", music_to_play), LogType::ERROR);
            return MA_ERROR;
        }
        music_list->headIn(music->getMusic());
    }

    auto* user_data(
        new UserData(engine, sound_to_register, sound_to_play, controller, music_list, music_playing, config)
    );

    ma_sound_set_end_callback(sound_to_play->sound_.get(), soundAtEndCallback, user_data);
    result = ma_sound_start(sound_to_play->sound_.get());
    if (result != MA_SUCCESS) {
        LOG(fmt::format("failed to start sound: {}", music_to_play), LogType::ERROR);
        return result;
    }
    sound_to_play->playing_.store(true, std::memory_order_release);
    LOG(fmt::format("started playing: {}", music_to_play), LogType::INFO);
    return MA_SUCCESS;
}

void cleanFunc(MaComponents* ma_comp, Controller* controller)
{
    while (true) {
        controller->flag_.wait(false, std::memory_order_acquire);
        controller->flag_.clear(std::memory_order_release);

        if (controller->getQuitFlag()) {
            ma_comp->sound_to_play_->uninit();
            LOG("uninitialized sound_to_play", LogType::INFO);

            ma_comp->sound_to_register_->uninit();
            LOG("uninitialized sound_to_register", LogType::INFO);

            utils::removeTmpFiles();
            LOG("removed tmp files", LogType::INFO);

            controller->cleaner_exited_.store(true, std::memory_order_release);
            LOG("cleaner exited", LogType::INFO);
            return;
        }
        if (controller->atEnd()) {
            if (!ma_comp->sound_to_play_->isPlaying() && ma_comp->sound_to_play_->isInitialized()) {
                ma_comp->sound_to_play_->uninit();
            }
            if (!ma_comp->sound_to_register_->isPlaying() && ma_comp->sound_to_register_->isInitialized()) {
                ma_comp->sound_to_register_->uninit();
            }
            controller->end_.store(false, std::memory_order_release);
        }
        if (controller->getError()) {
            ma_comp->sound_to_play_->uninit();
            ma_comp->sound_to_register_->uninit();
            controller->error_.store(false, std::memory_order_release);
        }
    }
}

void soundAtEndCallback(void* user_data, ma_sound* sound)
{
    auto* data = static_cast<UserData*>(user_data);
    if (!data) {
        LOG("got an invalid user_data", LogType::ERROR);
        throw std::runtime_error("got an invalid user_data");
    }
    const std::string& music_playing = *(data->music_playing_);

    if (!(data->config_->isRepetitive())) { data->music_list_->remove(music_playing); }
    data->music_list_->tailIn(music_playing);

    const std::string MUSIC_TO_PLAY = data->music_list_->headOut()->getMusic();

    ma_result result = playInternal(
        data->engine_,
        data->sound_to_play_,
        MUSIC_TO_PLAY,
        data->music_playing_,
        data->music_list_,
        data->sound_to_register_,
        data->controller_,
        data->config_
    );
    if (result != MA_SUCCESS) {
        LOG(fmt::format("failed to play music: {}", MUSIC_TO_PLAY), LogType::ERROR);
        fmt::print("failed in soundAtEndCallback\n");
        // data->sound_to_play_->uninit();
        // data->sound_to_register_->uninit();
        fmt::print("{}\n", data->sound_to_play_->isInitialized());
        fmt::print("{}\n", data->sound_to_register_->isInitialized());
        data->controller_->signalError();
    }
    else {
        data->controller_->signalEnd();
    }
    delete data;
}

void soundInitNotificationCallback(ma_async_notification* notification)
{
    auto* sound_init_notification = static_cast<SoundInitNotification*>(notification);
    auto* data                    = static_cast<SoundPack*>(sound_init_notification->data_);
    if (!data) {
        LOG("got an invalid data", LogType::ERROR);
        throw std::runtime_error("got an invalid data");
    }

    data->initialized_.store(true, std::memory_order_release);
    LOG("initialized sound", LogType::INFO);
}
