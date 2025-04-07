#include "core.h"
#include "config.h"
#include "log.h"
#include "miniaudio.h"
#include "music_list.h"
#include "thread_signals.h"
#include "utils.h"

#include <atomic>
#include <cstring>
#include <filesystem>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <thread>

// DO NOT LOG IN THIS FUNCTION
void dataCallback(ma_device* device, void* output, const void* input, ma_uint32 frame_count)
{
    auto* context = static_cast<Context*>(device->pUserData);
    if (!context) {
        return;
    }

    ma_uint32 frame_size = ma_get_bytes_per_frame(device->playback.format, device->playback.channels);
    if (context->is_audio_finished->load() || context->is_audio_paused->load()) {
        // clear output buffer
        std::memset(output, 0, frame_count * frame_size);
        return;
    }

    ma_decoder* current_decoder = *context->is_decoder1_used ? context->decoder1 : context->decoder2;
    if (!current_decoder) {
        return;
    }

    ma_uint64 frames_read = 0;
    ma_result result      = ma_decoder_read_pcm_frames(current_decoder, output, frame_count, &frames_read);
    if (result != MA_SUCCESS && result != MA_AT_END) {
        return;
    }

    if (result == MA_AT_END) {
        context->is_audio_finished->store(true);
        AudioFinishedCallbackSignals::getInstance().signalAudioFinished();
    }

    // if the read frame count is less than the requested frame count, fill the rest of the buffer with zeros
    if (frames_read < frame_count) {
        std::memset((ma_uint8*)output + frames_read * frame_size, 0, (frame_count - frames_read) * frame_size);
    }
}

void audioFinishedCallback()
{
    while (true) {
        auto& signals = AudioFinishedCallbackSignals::getInstance();
        signals.wait(false);
        if (signals.shouldQuit()) {
            LOG("audio finished callback quit", LogType::INFO);
            break;
        }
        if (signals.isAudioFinished()) {
            auto& core = CoreComponents::getInstance();

            const std::optional<std::string>& audio_to_play = core.getAudio();
            if (audio_to_play.has_value()) {
                ma_result result = core.play(audio_to_play.value());
                if (result != MA_SUCCESS) {
                    LOG(std::format("failed to play {}", audio_to_play.value()), LogType::ERROR, result);
                    core.quit();
                    return;
                }
            }
            else {
                LOG("failed to get next audio, try again", LogType::INFO);
                try {
                    core.setNextAudio();
                }
                catch (const std::exception& e) {
                    LOG(std::format("failed to set next audio: {}", e.what()), LogType::ERROR);
                    core.quit();
                    return;
                }
            }

            signals.reset();
        }
    }
}

ma_result CoreComponents::play(std::string_view audio)
{
    const std::string FILE_PATH = (std::filesystem::path(Config::USIC_LIBRARY) / audio).string();

    if (!std::filesystem::exists(FILE_PATH)) {
        LOG(std::format("audio file does not exist: {}", audio), LogType::ERROR);
        return MA_INVALID_DATA;
    }

    std::string wav_filename = Utils::getWAVFileName(audio);
    ma_result result         = MA_SUCCESS;
    if (std::ranges::find(cached_wav_files_, wav_filename) == cached_wav_files_.end() ||
        !std::filesystem::exists(wav_filename)) {
        result = decodeToWAV(FILE_PATH, wav_filename);
        if (result != MA_SUCCESS) {
            LOG("failed to decode to WAV", LogType::ERROR, result);
            return result;
        }
    }

    result = reinitUnusedDecoder(wav_filename);
    if (result != MA_SUCCESS) {
        LOG("failed to reinit unused decoder", LogType::ERROR, result);
        return result;
    }

    switchDecoder();
    removeWAVFile();
    current_playing_audio_ = audio;
    audio_finished_->store(false);
    music_list_->updateCurrent();
    setNextAudio();

    return result;
}

void CoreComponents::setNextAudio()
{
    switch (play_mode_) {
    case PlayMode::SEQUENCE:
        music_list_->forward();
        break;
    case PlayMode::SHUFFLE:
        music_list_->shuffle();
        break;
    case PlayMode::SINGLE:
        music_list_->single();
        break;
    }
}

std::optional<const std::string> CoreComponents::getAudio() const
{
    return music_list_->getMusic();
}

MusicList& CoreComponents::getMusicList() noexcept
{
    return *music_list_;
}

void CoreComponents::lockListInfoMutex()
{
    list_info_mutex_.lock();
}

void CoreComponents::unlockListInfoMutex()
{
    list_info_mutex_.unlock();
}

void CoreComponents::setPlayMode(PlayMode mode) noexcept
{
    play_mode_ = mode;
}

PlayMode CoreComponents::getPlayMode() const noexcept
{
    return play_mode_;
}

void CoreComponents::pauseOrResume() noexcept
{
    audio_paused_->store(!audio_paused_->load());
}

ma_result CoreComponents::setVolume(float volume)
{
    last_volume_         = current_volume_;
    float volume_clamped = volume < 0.0f ? 0.0f : (volume > 1.0f ? 1.0f : volume);
    ma_result result     = ma_device_set_master_volume(device_, volume_clamped);
    if (result != MA_SUCCESS) {
        LOG("failed to set master volume", LogType::ERROR);
        return result;
    }
    current_volume_ = volume_clamped;
    return result;
}

float CoreComponents::getCurrentVolume() const noexcept
{
    return current_volume_;
}

ma_result CoreComponents::mute()
{
    if (current_volume_ == 0.0f) {
        return setVolume(last_volume_);
    }
    else {
        return setVolume(0.0f);
    }
}

std::string CoreComponents::getCurrentPlayingAudio() const noexcept
{
    return current_playing_audio_;
}

std::optional<const float> CoreComponents::getCurrentProgress() const
{
    ma_decoder* current_decoder = *decoder1_used_ ? decoder1_ : decoder2_;
    ma_uint64 current_pcm_frame = 0;
    ma_uint64 length            = 0;
    ma_result result            = ma_decoder_get_length_in_pcm_frames(current_decoder, &length);
    if (result != MA_SUCCESS) {
        LOG("failed to get length in PCM frames", LogType::ERROR, result);
        return std::nullopt;
    }

    result = ma_decoder_get_cursor_in_pcm_frames(current_decoder, &current_pcm_frame);
    if (result != MA_SUCCESS) {
        LOG("failed to get current PCM frame", LogType::ERROR, result);
        return std::nullopt;
    }

    float progress = static_cast<float>(current_pcm_frame) / static_cast<float>(length);
    return progress;
}

ma_decoder* CoreComponents::getCurrentDecoder() const noexcept
{
    return *decoder1_used_ ? decoder1_ : decoder2_;
}

ma_uint32 CoreComponents::getSampleRate() const noexcept
{
    return sample_rate_;
}

bool CoreComponents::shouldQuit() const noexcept
{
    return should_quit_;
}

void CoreComponents::quit()
{
    should_quit_ = true;
    AudioFinishedCallbackSignals::getInstance().quit();
}

CoreComponents::CoreComponents()
    : device_(new ma_device)
    , decoder1_(new ma_decoder)
    , decoder2_(new ma_decoder)
    , encoder_(new ma_encoder)
    , decoder1_used_(new bool{false})
    , audio_finished_(new std::atomic<bool>{true})
    , audio_paused_(new std::atomic<bool>{false})
    , music_list_(new MusicList)
{
    ma_device_config device_config  = ma_device_config_init(ma_device_type_playback);
    device_config.playback.format   = format_;
    device_config.playback.channels = channels_;
    device_config.sampleRate        = sample_rate_;
    device_config.dataCallback      = dataCallback;

    static Context context{
        .decoder1          = decoder1_,
        .decoder2          = decoder2_,
        .is_decoder1_used  = decoder1_used_,
        .is_audio_finished = audio_finished_,
        .is_audio_paused   = audio_paused_,
    };
    device_config.pUserData = &context;

    ma_result result = ma_device_init(nullptr, &device_config, device_);
    if (result != MA_SUCCESS) {
        LOG("failed to initialize device", LogType::ERROR, result);
        deletePtr();
        std::exit(1);
    }

    result = ma_device_start(device_);
    if (result != MA_SUCCESS) {
        LOG("failed to start device", LogType::ERROR, result);
        ma_device_uninit(device_);
        deletePtr();
        std::exit(1);
    }

    decoder_config_ = ma_decoder_config_init(format_, channels_, sample_rate_);
    encoder_config_ = ma_encoder_config_init(ma_encoding_format_wav, format_, channels_, sample_rate_);

    try {
        initMusicList();

        // register audio finished callback
        std::thread audio_finished_thread{audioFinishedCallback};
        audio_finished_thread.detach();
    }
    catch (const std::exception& e) {
        LOG(e.what(), LogType::ERROR);
        ma_device_uninit(device_);
        deletePtr();
        std::exit(1);
    }

    initPlayMode();
}

CoreComponents::~CoreComponents()
{
    ma_device_uninit(device_);
    ma_decoder_uninit(decoder1_);
    ma_decoder_uninit(decoder2_);
    removeAllWAVFiles();
    deletePtr();
}

void CoreComponents::initMusicList()
{
    if (Config::DEFAULT_PLAYLIST.empty()) {
        const std::string DEFAULT_LIST = Utils::createTmpDefaultList();
        music_list_->load(DEFAULT_LIST);
        if (music_list_->isEmpty()) {
            LOG("failed to load default music list while user defined default music list is not specified",
                LogType::ERROR);
            throw std::runtime_error("failed to load default music list");
        }
    }
    else {
        music_list_->load(std::format("{}{}", RuntimePath::PLAYLISTS_PATH, Config::DEFAULT_PLAYLIST));
        if (music_list_->isEmpty()) {
            LOG(std::format(
                    "failed to load music list {}{}, or list is empty",
                    RuntimePath::PLAYLISTS_PATH,
                    Config::DEFAULT_PLAYLIST
                ),
                LogType::ERROR);
            throw std::runtime_error("failed to load default music list");
        }
    }
}

void CoreComponents::initPlayMode() noexcept
{
    switch (Config::DEFAULT_PLAY_MODE) {
    case 0:
        play_mode_ = PlayMode::SEQUENCE;
        break;
    case 1:
        play_mode_ = PlayMode::SHUFFLE;
        break;
    case 2:
        play_mode_ = PlayMode::SINGLE;
        break;
    default:
        LOG("invalid play mode, fallback to sequence", LogType::INFO);
        play_mode_ = PlayMode::SEQUENCE;
        break;
    }
}

void CoreComponents::switchDecoder() noexcept
{
    *decoder1_used_ = !*decoder1_used_;
}

ma_result CoreComponents::decodeToWAV(std::string_view input_file, std::string_view output_file)
{
    ma_decoder decoder;
    ma_result result = ma_decoder_init_file(input_file.data(), &decoder_config_, &decoder);
    if (result != MA_SUCCESS) {
        LOG(std::format("failed to initialize decoder for file: {}", input_file), LogType::ERROR, result);
        return result;
    }

    ma_uint64 pcm_frame_count = 0;
    result                    = ma_decoder_get_length_in_pcm_frames(&decoder, &pcm_frame_count);
    if (result != MA_SUCCESS) {
        LOG("failed to get length in PCM frames", LogType::ERROR, result);
        ma_decoder_uninit(&decoder);
        return result;
    }

    // create an empty wav file for the output
    try {
        Utils::createFile(output_file);
    }
    catch (const std::exception& e) {
        LOG(std::format("failed to create file: {}, {}", output_file, e.what()), LogType::ERROR);
        ma_decoder_uninit(&decoder);
        return MA_ERROR;
    }

    result = ma_encoder_init_file(output_file.data(), &encoder_config_, encoder_);
    if (result != MA_SUCCESS) {
        LOG(std::format("failed to initialize encoder for file: {}", output_file), LogType::ERROR, result);
        ma_decoder_uninit(&decoder);
        return result;
    }

    const size_t FRAME_COUNT = 4096;

    ma_uint32 frame_size = ma_get_bytes_per_frame(decoder.outputFormat, decoder.outputChannels);
    void* pcm_frames     = malloc(frame_size * FRAME_COUNT);
    if (pcm_frames == nullptr) {
        LOG("failed to allocate memory", LogType::ERROR);
        ma_decoder_uninit(&decoder);
        ma_encoder_uninit(encoder_);
        return MA_OUT_OF_MEMORY;
    }

    ma_uint64 frames_read    = 0;
    ma_uint64 frames_written = 0;
    while (true) {
        ma_decoder_read_pcm_frames(&decoder, pcm_frames, FRAME_COUNT, &frames_read);
        if (frames_read == 0)
            break;

        ma_encoder_write_pcm_frames(encoder_, pcm_frames, frames_read, &frames_written);
    }

    cached_wav_files_.push_back(output_file.data());

    free(pcm_frames);
    ma_encoder_uninit(encoder_);
    ma_decoder_uninit(&decoder);

    return result;
}

ma_result CoreComponents::reinitUnusedDecoder(std::string_view filename)
{
    ma_result result = MA_SUCCESS;
    if (*decoder1_used_) {
        result = Utils::reinitDecoder(filename, &decoder_config_, decoder2_);
        if (result != MA_SUCCESS) {
            LOG("failed to reinitialize decoder2", LogType::ERROR, result);
            return result;
        }

        result = Utils::seekToStart(decoder2_);
        if (result != MA_SUCCESS) {
            LOG("failed to seek to start for decoder2", LogType::ERROR, result);
            return result;
        }
    }
    else {
        result = Utils::reinitDecoder(filename, &decoder_config_, decoder1_);
        if (result != MA_SUCCESS) {
            LOG("failed to reinitialize decoder1", LogType::ERROR, result);
            return result;
        }

        result = Utils::seekToStart(decoder1_);
        if (result != MA_SUCCESS) {
            LOG("failed to seek to start for decoder1", LogType::ERROR, result);
            return result;
        }
    }
    return result;
};

void CoreComponents::removeWAVFile()
{
    if (cached_wav_files_.size() > Config::NUM_CACHED_WAV_FILES) {
        std::remove(cached_wav_files_.front().data());
        LOG(std::format("removed {}", cached_wav_files_.front()), LogType::INFO);
        cached_wav_files_.pop_front();
    }
}

void CoreComponents::removeAllWAVFiles()
{
    std::string temp_dir           = std::filesystem::temp_directory_path().string();
    std::filesystem::path wav_path = std::filesystem::path(temp_dir) / "usic";
    std::filesystem::remove_all(wav_path);
    LOG("removed all cached wav files", LogType::INFO);
}

void CoreComponents::deletePtr()
{
    delete device_;
    delete decoder1_;
    delete decoder2_;
    delete encoder_;
    delete decoder1_used_;
    delete audio_finished_;
    delete audio_paused_;
    delete music_list_;
}

CoreComponents& CoreComponents::getInstance() noexcept
{
    static CoreComponents instance;
    return instance;
}
