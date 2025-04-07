#include "commands.h"
#include "config.h"
#include "core.h"
#include "log.h"
#include "miniaudio.h"
#include "utils.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Commands
{

void load(std::string_view list)
{
    auto& core       = CoreComponents::getInstance();
    auto& music_list = core.getMusicList();
    try {
        music_list.load(list);
    }
    catch (const std::exception& e) {
        LOG(e.what(), LogType::ERROR);
        throw;
    }
}

ma_result play(std::string_view audio)
{
    return CoreComponents::getInstance().play(audio);
}

ma_result playLater(std::string_view audio)
{
    auto& music_list = CoreComponents::getInstance().getMusicList();
    try {
        if (!music_list.moveTo(audio)) {
            music_list.insertAfterCurrent(audio);
            music_list.forward();
        }
    }
    catch (const std::exception& e) {
        LOG(e.what(), LogType::ERROR);
        return MA_INVALID_DATA;
    }
    return MA_SUCCESS;
}

static ma_result seekCurrentDecoderToEnd()
{
    auto& core            = CoreComponents::getInstance();
    auto* current_decoder = core.getCurrentDecoder();

    ma_result result = Utils::seekToEnd(current_decoder);
    if (result != MA_SUCCESS) {
        LOG("failed to seek to end for current decoder", LogType::ERROR, result);
    }
    return result;
}

ma_result playNext()
{
    return seekCurrentDecoderToEnd();
}

ma_result playPrev()
{
    auto& core = CoreComponents::getInstance();
    core.getMusicList().backward();
    return seekCurrentDecoderToEnd();
}

void pause()
{
    CoreComponents::getInstance().pauseOrResume();
}

static std::optional<unsigned int> getCursorInSeconds()
{
    auto& core                  = CoreComponents::getInstance();
    auto* current_decoder       = core.getCurrentDecoder();
    auto sample_rate            = core.getSampleRate();
    ma_uint64 current_pcm_frame = 0;
    ma_result result            = ma_decoder_get_cursor_in_pcm_frames(current_decoder, &current_pcm_frame);
    if (result != MA_SUCCESS) {
        LOG("failed to get current PCM frame", LogType::ERROR, result);
        return std::nullopt;
    }
    return static_cast<unsigned int>(current_pcm_frame) / static_cast<unsigned int>(sample_rate);
}

static ma_result seekCurrentDecoderTo(unsigned int seconds)
{
    auto& core            = CoreComponents::getInstance();
    auto* current_decoder = core.getCurrentDecoder();
    auto sample_rate      = core.getSampleRate();

    ma_uint64 length = 0;
    ma_result result = ma_decoder_get_length_in_pcm_frames(current_decoder, &length);
    if (result != MA_SUCCESS) {
        LOG("failed to get length in PCM frames", LogType::ERROR, result);
        return result;
    }
    if (seconds * sample_rate > length) {
        return seekCurrentDecoderToEnd();
    }

    result = ma_decoder_seek_to_pcm_frame(current_decoder, seconds * sample_rate);
    if (result != MA_SUCCESS) {
        LOG("failed to seek to seconds for current decoder", LogType::ERROR, result);
    }
    return result;
}

ma_result seekForward()
{
    auto seconds = getCursorInSeconds();
    if (!seconds.has_value()) {
        LOG("failed to get cursor in seconds", LogType::ERROR);
        return MA_INVALID_DATA;
    }

    return seekCurrentDecoderTo(seconds.value() + Config::STEP_SECONDS);
}

ma_result seekBackward()
{
    auto seconds = getCursorInSeconds();
    if (!seconds.has_value()) {
        LOG("failed to get cursor in seconds", LogType::ERROR);
        return MA_INVALID_DATA;
    }
    if (seconds.value() < Config::STEP_SECONDS) {
        return seekCurrentDecoderTo(0);
    }

    return seekCurrentDecoderTo(seconds.value() - Config::STEP_SECONDS);
}

ma_result seekTo(std::string_view time)
{
    auto seconds = Utils::timeStrToSec(time);
    if (!seconds.has_value()) {
        LOG("failed to parse time", LogType::ERROR);
        return MA_INVALID_ARGS;
    }

    return seekCurrentDecoderTo(seconds.value());
}

static const std::string makeProgressBar(float progress)
{
    std::string bar(Config::PROGRESS_BAR_LENGTH, Config::BAR_ELEMENT);
    int cursor  = static_cast<int>(Config::PROGRESS_BAR_LENGTH * progress);
    bar[cursor] = Config::CURSOR_ELEMENT;
    return bar;
}

std::optional<const std::string> getProgressBar()
{
    auto& core                              = CoreComponents::getInstance();
    ma_uint32 sample_rate                   = core.getSampleRate();
    ma_decoder* current_decoder             = core.getCurrentDecoder();
    const std::string CURRENT_PLAYING_AUDIO = core.getCurrentPlayingAudio();

    ma_uint64 length                      = 0;
    ma_result result                      = ma_decoder_get_length_in_pcm_frames(current_decoder, &length);
    std::optional<std::string> length_str = Utils::secToTimeStr(length / sample_rate);
    if (result != MA_SUCCESS || !length_str.has_value()) {
        LOG("failed to get length in PCM frames", LogType::ERROR, result);
        return std::nullopt;
    }

    ma_uint64 current_pcm_frame            = 0;
    result                                 = ma_decoder_get_cursor_in_pcm_frames(current_decoder, &current_pcm_frame);
    std::optional<std::string> current_str = Utils::secToTimeStr(current_pcm_frame / sample_rate);
    if (result != MA_SUCCESS || !current_str.has_value()) {
        LOG("failed to get current PCM frame", LogType::ERROR, result);
        return std::nullopt;
    }

    float progress        = static_cast<float>(current_pcm_frame) / static_cast<float>(length);
    const std::string BAR = makeProgressBar(progress);

    std::string progress_bar =
        std::format("{}\n{} {}/{}", CURRENT_PLAYING_AUDIO, BAR, current_str.value(), length_str.value());
    return progress_bar;
}

ma_result volumeUp()
{
    return CoreComponents::getInstance().setVolume(
        CoreComponents::getInstance().getCurrentVolume() + Config::VOLUME_STEP
    );
}

ma_result volumeDown()
{
    return CoreComponents::getInstance().setVolume(
        CoreComponents::getInstance().getCurrentVolume() - Config::VOLUME_STEP
    );
}

ma_result setVolume(std::string_view volume)
{
    float volume_value = 0.0f;
    try {
        volume_value = std::stof(volume.data());
    }
    catch (const std::exception& e) {
        LOG(e.what(), LogType::ERROR);
        return MA_INVALID_ARGS;
    }

    return CoreComponents::getInstance().setVolume(volume_value);
}

float getVolume()
{
    return CoreComponents::getInstance().getCurrentVolume();
}

ma_result mute()
{
    return CoreComponents::getInstance().mute();
}

void setMode(PlayMode mode)
{
    CoreComponents::getInstance().setPlayMode(mode);
}

PlayMode getMode()
{
    return CoreComponents::getInstance().getPlayMode();
}

std::vector<std::string> getList()
{
    return CoreComponents::getInstance().getMusicList().getList();
}

void quit()
{
    CoreComponents::getInstance().quit();
}

} // namespace Commands
