#include "core.h"
#include "log.h"
#include "miniaudio.h"
#include "utils.h"

#include <atomic>
#include <cstring>
#include <filesystem>
#include <format>
#include <string>
#include <string_view>
#include <thread>

/// dataCallback for ma_device
// DO NOT LOG IN THIS FUNCTION
void dataCallback(ma_device* device, void* output, const void* input, ma_uint32 frame_count)
{
    auto* context = static_cast<Context*>(device->pUserData);
    if (!context) {
        return;
    }

    ma_uint32 frame_size = ma_get_bytes_per_frame(device->playback.format, device->playback.channels);
    if (context->is_audio_finished->load()) {
        // clear output buffer
        std::memset(output, 0, frame_count * frame_size);
    }

    ma_decoder* current_decoder = *context->is_decoder1_used ? context->decoder1 : context->decoder2;
    if (!current_decoder) {
        return;
    }

    ma_uint64 frames_read = 0;
    ma_result result      = ma_decoder_read_pcm_frames(current_decoder, output, frame_count, &frames_read);
    if (result != MA_SUCCESS) {
        return;
    }

    // if the read frame count is less than the requested frame count, fill the rest of the buffer with zeros
    if (frames_read < frame_count) {
        std::memset((ma_uint8*)output + frames_read * frame_size, 0, (frame_count - frames_read) * frame_size);
    }

    if (frames_read == 0) {
        context->is_audio_finished->store(true);
    }
}

CoreComponents& CoreComponents::get() noexcept
{
    return g_core;
}

ma_result CoreComponents::play(std::string_view filename)
{
    if (!std::filesystem::exists(filename)) {
        LOG(std::format("audio file does not exist: {}", filename), LogType::ERROR);
        return MA_INVALID_DATA;
    }

    std::string wav_filename = Utils::getWAVFileName(filename);
    Utils::createFile(wav_filename);

    // Decoding in background, so that it doesn't block the main thread.
    // sound can be played even if decoding is still in progress
    std::thread decode_thread([this, filename, wav_filename]() {
        ma_result decode_result = decodeToWAV(filename, wav_filename);
        if (decode_result != MA_SUCCESS) {
            LOG("background decoding failed", LogType::ERROR, decode_result);
        }
        LOG("background decoding finished", LogType::INFO);
    });
    decode_thread.detach();

    ma_result result = reinitUnusedDecoder(wav_filename);
    if (result != MA_SUCCESS) {
        LOG("failed to reinit unused decoder", LogType::ERROR, result);
        return result;
    }

    switchDecoder();
    current_playing_audio_ = filename;
    return result;
}

ma_result CoreComponents::pauseOrResume()
{
    ma_result result = MA_SUCCESS;
    if (is_audio_paused_->load()) {
        result = ma_device_start(device_);
        if (result != MA_SUCCESS) {
            LOG("failed to start device", LogType::ERROR, result);
            return result;
        }
    }
    else {
        result = ma_device_stop(device_);
        if (result != MA_SUCCESS) {
            LOG("failed to stop device", LogType::ERROR, result);
            return result;
        }
    }
    is_audio_paused_->store(!is_audio_paused_->load());
    return result;
}

ma_result CoreComponents::moveCursorToStart()
{
    ma_result result = MA_SUCCESS;
    if (*is_decoder1_used_) {
        result = Utils::seekToStart(decoder1_);
        if (result != MA_SUCCESS) {
            LOG("failed to seek to start for decoder1", LogType::ERROR, result);
            return result;
        }
    }
    else {
        result = Utils::seekToStart(decoder2_);
        if (result != MA_SUCCESS) {
            LOG("failed to seek to start for decoder2", LogType::ERROR, result);
            return result;
        }
    }
    return result;
}

ma_result CoreComponents::moveCursorToEnd()
{
    ma_result result = MA_SUCCESS;
    if (*is_decoder1_used_) {
        result = Utils::seekToEnd(decoder1_);
        if (result != MA_SUCCESS) {
            LOG("failed to seek to end for decoder1", LogType::ERROR, result);
            return result;
        }
    }
    else {
        result = Utils::seekToEnd(decoder2_);
        if (result != MA_SUCCESS) {
            LOG("failed to seek to end for decoder2", LogType::ERROR, result);
            return result;
        }
    }
    return result;
}

void CoreComponents::clearWAVFile()
{
    if (is_audio_finished_->load()) {
        std::string wav_filename = Utils::getWAVFileName(current_playing_audio_);
        std::filesystem::remove(wav_filename);
    }
}

CoreComponents::CoreComponents()
    : device_(new ma_device)
    , decoder1_(new ma_decoder)
    , decoder2_(new ma_decoder)
    , encoder_(new ma_encoder)
    , is_decoder1_used_(new bool{false})
    , is_audio_finished_(new std::atomic<bool>{true})
    , is_audio_paused_(new std::atomic<bool>{false})
{
    ma_device_config device_config  = ma_device_config_init(ma_device_type_playback);
    device_config.playback.format   = format_;
    device_config.playback.channels = channels_;
    device_config.sampleRate        = sample_rate_;
    device_config.dataCallback      = dataCallback;

    static Context context{
        .decoder1          = decoder1_,
        .decoder2          = decoder2_,
        .is_decoder1_used  = is_decoder1_used_,
        .is_audio_finished = is_audio_finished_,
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
}

CoreComponents::~CoreComponents()
{
    ma_device_uninit(device_);
    ma_decoder_uninit(decoder1_);
    ma_decoder_uninit(decoder2_);
    deletePtr();
}

void CoreComponents::switchDecoder() noexcept
{
    *is_decoder1_used_ = !*is_decoder1_used_;
    is_audio_finished_->store(false);
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

    free(pcm_frames);
    ma_encoder_uninit(encoder_);
    ma_decoder_uninit(&decoder);

    return result;
}

ma_result CoreComponents::reinitUnusedDecoder(std::string_view filename)
{
    ma_result result = MA_SUCCESS;
    if (*is_decoder1_used_) {
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

void CoreComponents::deletePtr()
{
    delete device_;
    delete decoder1_;
    delete decoder2_;
    delete encoder_;
    delete is_decoder1_used_;
    delete is_audio_finished_;
    delete is_audio_paused_;
}

CoreComponents CoreComponents::g_core;
