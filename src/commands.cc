#include "commands.h"
#include "config.h"
#include "core.h"
#include "fmt/core.h"
#include "music_list.h"
#include "runtime.h"
#include "utils.h"
#include <cmath>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

void Progress::init(std::string_view str, float percent)
{
    this->str_     = std::move(str);
    this->percent_ = percent;
}

const std::string Progress::makeBar()
{
    int n = percent_ * PROGRESS_BAR_LENGTH;
    std::string bar(PROGRESS_BAR_LENGTH, BAR_ELEMENT);
    bar[n] = CURSOR_ELEMENT;
    return fmt::format("{} {}", bar, str_);
}

static ma_sound* getPlayingSound(MaComponents* ma_comp)
{
    ma_sound* sound = nullptr;
    if (!ma_comp) {
        log("got an invalid MaComponents", LogType::ERROR, __func__);
        return sound;
    }
    if (ma_comp->sound_to_play_->isPlaying() && !ma_comp->sound_to_register_->isPlaying()) {
        sound = ma_comp->sound_to_play_->sound_.get();
    }
    else if (!ma_comp->sound_to_play_->isPlaying() && ma_comp->sound_to_register_->isPlaying()) {
        sound = ma_comp->sound_to_register_->sound_.get();
    }
    return sound;
}

namespace commands
{

void load(std::string_view list_path, MusicList* music_list, Config* config)
{
    music_list->load(list_path, config);
}

ma_result play(
    MaComponents* ma_comp,
    std::string_view music_to_play,
    std::string* music_playing,
    MusicList* music_list,
    QuitController* quit_controller,
    Config* config
)
{
    ma_result result;
    if (!ma_comp) {
        log("got an invalid MaComponents", LogType::ERROR, __func__);
        return MA_ERROR;
    }
    if (!ma_comp->sound_to_play_->isInitialized() && !ma_comp->sound_to_register_->isInitialized()) {
        result = playInternal(
            ma_comp->engine_.get(),
            ma_comp->sound_to_play_.get(),
            music_to_play,
            music_playing,
            music_list,
            ma_comp->sound_to_register_.get(),
            quit_controller,
            config->getRandomPtr(),
            config->getRepetitivePtr()
        );
        if (result != MA_SUCCESS) {
            log(fmt::format("failed to play music: {}", music_to_play), LogType::ERROR, __func__);
        }
    }
    else {
        playLater(config, music_to_play, music_list);
        result = playNext(ma_comp);
        if (result != MA_SUCCESS) {
            log(fmt::format("failed to play music: {}", music_to_play), LogType::ERROR, __func__);
        }
    }
    return result;
}

void playLater(Config* config, std::string_view music, MusicList* music_list)
{
    if (!music_list) {
        log("got an invalid music_list", LogType::ERROR, __func__);
        return;
    }
    if (!config->isRepetitive()) {
        // fmt::print("{} removed: {}\n", music, music_list->remove(music));
        music_list->remove(music);
    }
    music_list->headIn(music);
}

ma_result playNext(MaComponents* ma_comp)
{
    ma_sound* sound = getPlayingSound(ma_comp);
    if (sound == nullptr) {
        log("failed to get playing sound", LogType::ERROR, __func__);
        return MA_ERROR;
    }

    ma_uint64 length = 0;
    ma_result result = ma_sound_get_length_in_pcm_frames(sound, &length);
    if (result != MA_SUCCESS) {
        log("failed to get length in pcm frames", LogType::ERROR, __func__);
        return result;
    }
    result = ma_sound_seek_to_pcm_frame(sound, length);
    if (result != MA_SUCCESS) {
        log("failed to seek to pcm frame", LogType::ERROR, __func__);
        return result;
    }
    if (ma_sound_is_playing(sound) == 0U) {
        result = ma_sound_start(sound);
        if (result != MA_SUCCESS) {
            log("failed to start sound", LogType::ERROR, __func__);
            return result;
        }
    }
    return MA_SUCCESS;
}

ma_result playPrev(MaComponents* ma_comp, MusicList* music_list)
{
    if (!music_list || music_list->isEmpty()) {
        log("got an invalid music_list", LogType::ERROR, __func__);
        return MA_ERROR;
    }
    auto prev_music = music_list->tailOut();
    if (prev_music == nullptr) {
        log("failed to get prev music", LogType::ERROR, __func__);
        return MA_ERROR;
    }
    music_list->headIn(prev_music->getMusic());
    return playNext(ma_comp);
}

ma_result pause(MaComponents* ma_comp)
{
    ma_sound* sound = getPlayingSound(ma_comp);
    if (sound == nullptr) {
        log("failed to get playing sound", LogType::ERROR, __func__);
        return MA_ERROR;
    }

    ma_result result;
    if (ma_sound_is_playing(sound) != 0U) {
        // if isPlaying, pause
        result = ma_sound_stop(sound);
        if (result != MA_SUCCESS) {
            log("failed to stop the playing sound", LogType::ERROR, __func__);
            return result;
        }
    }
    else {
        // else resume
        result = ma_sound_start(sound);
        if (result != MA_SUCCESS) {
            log("failed to start the stopped sound", LogType::ERROR, __func__);
            return result;
        }
    }
    return MA_SUCCESS;
}

static ma_result moveCursor(MaComponents* ma_comp, int seconds)
{
    ma_sound* sound = getPlayingSound(ma_comp);
    if (sound == nullptr) {
        log("failed to get playing sound", LogType::ERROR, __func__);
        return MA_ERROR;
    }

    // Get current position of cursor and total length
    ma_uint64 cursor = 0;
    ma_uint64 length = 0;
    ma_result result = ma_sound_get_cursor_in_pcm_frames(sound, &cursor);
    if (result != MA_SUCCESS) {
        log("failed to get cursor in pcm frames", LogType::ERROR, __func__);
        return result;
    }
    result = ma_sound_get_length_in_pcm_frames(sound, &length);
    if (result != MA_SUCCESS) {
        log("failed to get length in pcm frames", LogType::ERROR, __func__);
        return result;
    }

    // Calculate the position of the cursor after moving
    ma_uint64 moved_cursor = 0;
    if (seconds > 0) {
        int new_cursor = cursor + (ma_engine_get_sample_rate(ma_comp->engine_.get()) * seconds);
        moved_cursor   = new_cursor < length ? new_cursor : length;
    }
    else {
        seconds        = -seconds;
        int new_cursor = cursor - (ma_engine_get_sample_rate(ma_comp->engine_.get()) * seconds);

        moved_cursor = new_cursor > 0 ? new_cursor : 0;
    }

    // Move cursor
    result = ma_sound_seek_to_pcm_frame(sound, moved_cursor);
    if (result != MA_SUCCESS) {
        log("failed to seek to pcm frame", LogType::ERROR, __func__);
        return result;
    }
    return result;
}

ma_result cursorForward(MaComponents* ma_comp)
{
    return moveCursor(ma_comp, STEP_SECONDS);
}
ma_result cursorBackward(MaComponents* ma_comp)
{
    return moveCursor(ma_comp, -STEP_SECONDS);
}

ma_result setCursor(MaComponents* ma_comp, std::string_view time)
{
    ma_sound* sound = getPlayingSound(ma_comp);
    if (sound == nullptr) {
        log("failed to get playing sound", LogType::ERROR, __func__);
        return MA_ERROR;
    }

    // Convert the time string to seconds
    std::optional<int> optional_destination = utils::timeStrToSec(time);
    if (!optional_destination.has_value()) {
        log(fmt::format("failed to convert time string \"{}\" to seconds", time), LogType::ERROR, __func__);
        return MA_INVALID_ARGS;
    }

    // Get the total length of the sound
    float length     = 0;
    ma_result result = ma_sound_get_length_in_seconds(sound, &length);
    if (result != MA_SUCCESS) { return result; }

    int len_int = static_cast<int>(length); // must be rounded down

    // Calculate the destination
    int destination = optional_destination.value() < 0         ? 0
                      : optional_destination.value() > len_int ? len_int
                                                               : optional_destination.value();

    result = ma_sound_seek_to_pcm_frame(sound, (ma_engine_get_sample_rate(ma_comp->engine_.get()) * destination));

    if (result != MA_SUCCESS) {
        log("failed to seek to pcm frame", LogType::ERROR, __func__);
        return result;
    }
    return result;
}

ma_result getCurrentProgress(MaComponents* ma_comp, Progress* current_progress)
{
    ma_sound* sound = getPlayingSound(ma_comp);
    if (sound == nullptr) {
        log("failed to get playing sound", LogType::ERROR, __func__);
        return MA_ERROR;
    }

    float cursor   = 0.0F;
    float duration = 0.0F;

    // Get the current cursor position
    ma_result result = ma_sound_get_cursor_in_seconds(sound, &cursor);
    if (result != MA_SUCCESS) { return result; }

    // Get the duration of the sound
    result = ma_sound_get_length_in_seconds(sound, &duration);
    if (result != MA_SUCCESS) {
        log("failed to get length in seconds", LogType::ERROR, __func__);
        return result;
    }

    cursor   = round(cursor);
    duration = round(duration);

    std::optional<std::string> cursor_str   = utils::secToTimeStr(cursor);
    std::optional<std::string> duration_str = utils::secToTimeStr(duration);
    if (!cursor_str.has_value()) {
        log(fmt::format(
                "failed to convert cursor position \"{}\" to time string: "
                "cursor position "
                "should be >= 0",
                cursor
            ),
            LogType::ERROR,
            __func__);
        return MA_ERROR;
    }
    if (!duration_str.has_value()) {
        log(fmt::format(
                "failed to convert duration \"{}\" to time string: duration "
                "should be >= 0",
                duration
            ),
            LogType::ERROR,
            __func__);
        return MA_ERROR;
    }

    std::string str = fmt::format("{}/{}", cursor_str.value(), duration_str.value());
    current_progress->init(str, cursor / duration);
    return result;
}

/*
 * Adjust the volume of the engine, which should be 0.0-1.0.
 * The parameter "diff" indicates the step length.
 */
static ma_result adjustVolume(ma_engine* engine, float diff)
{
    ma_device* device = ma_engine_get_device(engine);

    // Get current volume of the engine
    float current_volume = 0.0;
    ma_result result     = ma_device_get_master_volume(device, &current_volume);
    if (result != MA_SUCCESS) {
        log("failed to get master volume", LogType::ERROR, __func__);
        return result;
    }

    // Calculate the new volume
    float new_volume = current_volume + diff;
    new_volume       = new_volume < 0.0F ? 0.0F : new_volume > 1.0F ? 1.0F : new_volume;

    // Set volume
    result = ma_device_set_master_volume(device, new_volume);
    if (result != MA_SUCCESS) {
        log("failed to set master volume", LogType::ERROR, __func__);
        return result;
    }
    return MA_SUCCESS;
}

ma_result volumeUp(ma_engine* engine)
{
    return adjustVolume(engine, VOLUME_STEP);
}

ma_result volumeDown(ma_engine* engine)
{
    return adjustVolume(engine, -VOLUME_STEP);
}

ma_result setVolume(ma_engine* engine, std::string_view volume_str)
{
    // Convert the volume string to float
    float volume;
    try {
        volume = std::stof(volume_str.data());
    }
    catch (const std::invalid_argument& e) {
        log(fmt::format("got an invalid argument: {}", e.what()), LogType::ERROR, __func__);
        return MA_ERROR;
    }
    catch (const std::out_of_range& e) {
        log(fmt::format("the argument is out of range: {}", e.what()), LogType::ERROR, __func__);
        return MA_ERROR;
    }
    volume = volume < 0.0F ? 0.0F : volume > 1.0F ? 1.0F : volume;

    // Set volume
    ma_device* device = ma_engine_get_device(engine);
    ma_result result  = ma_device_set_master_volume(device, volume);
    if (result != MA_SUCCESS) {
        log("failed to set master volume", LogType::ERROR, __func__);
        return result;
    }
    return MA_SUCCESS;
}

/*
 * Get the volume of the engine and print it as 0-100%
 */
ma_result getVolume(ma_engine* engine, float* volume)
{
    ma_device* device = ma_engine_get_device(engine);
    return ma_device_get_master_volume(device, volume);
}

ma_result mute(ma_engine* engine)
{
    ma_device* device = ma_engine_get_device(engine);

    float current_volume = 0.0;
    ma_result result     = ma_device_get_master_volume(device, &current_volume);
    if (result != MA_SUCCESS) {
        log("failed to get master volume", LogType::ERROR, __func__);
        return result;
    }
    if (current_volume == 0.0F) {
        result = ma_device_set_master_volume(device, 1.0F);
        if (result != MA_SUCCESS) {
            log("failed to set master volume", LogType::ERROR, __func__);
            return result;
        }
    }
    else {
        result = ma_device_set_master_volume(device, 0.0F);
        if (result != MA_SUCCESS) {
            log("failed to set master volume", LogType::ERROR, __func__);
            return result;
        }
    }
    return MA_SUCCESS;
}

void setRandom(Config* config)
{
    config->toggleRandom();
}
void setRepetitive(Config* config)
{
    config->toggleRepetitive();
}

std::vector<std::string> getList(MusicList* music_list)
{
    return music_list->getList();
}

static ma_result stop(MaComponents* ma_comp)
{
    ma_sound* sound = getPlayingSound(ma_comp);
    if (sound == nullptr) {
        log("failed to get playing sound", LogType::ERROR, __func__);
        return MA_ERROR;
    }

    ma_result result;
    result = ma_sound_stop(sound);
    if (result != MA_SUCCESS) {
        log("failed to stop the playing sound", LogType::ERROR, __func__);
        return result;
    }
    return MA_SUCCESS;
}

void quit(MaComponents* ma_comp, QuitController* quit_controller)
{
    if (stop(ma_comp) != MA_SUCCESS) { log("failed to stop music", LogType::ERROR, __func__); }
    if (ma_comp->sound_to_play_->isInitialized()) {
        ma_comp->sound_to_play_->uninit();
        log("uninitialized pSound_to_play", LogType::INFO, __func__);
    }
    if (ma_comp->sound_to_register_->isInitialized()) {
        ma_comp->sound_to_register_->uninit();
        log("uninitialized pSound_to_register", LogType::INFO, __func__);
    }
    quit_controller->quit();
}

} // namespace commands