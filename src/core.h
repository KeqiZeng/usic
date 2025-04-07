#pragma once

#include "miniaudio.h"
#include "music_list.h"
#include <atomic>
#include <deque>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

/**
 * This callback function is responsible for audio data processing, which is required by miniaudio device.
 *
 * @param device Pointer to miniaudio device.
 * @param output Pointer to the buffer where audio data should be written.
 * @param input Pointer to the buffer containing input audio data (unused in this context).
 * @param frame_count Number of PCM frames to process at a time.
 */
void dataCallback(ma_device* device, void* output, const void* input, ma_uint32 frame_count);

/**
 * Continuously monitors the audio playback status and switches to the next track when the current one finishes.
 * It relies on the AudioFinishedCallbackSignals to determine when the audio is finished or if it should quit.
 */
void audioFinishedCallback();

/**
 * This structure contains the necessary information required by the dataCallback function.
 *
 * @var decoder1 Pointer to the first decoder.
 * @var decoder2 Pointer to the second decoder.
 * @var is_decoder1_used Pointer to a boolean indicating if decoder1 is being used now.
 * @var is_audio_finished Pointer to a boolean indicating if the audio playback is finished.
 * @var is_audio_paused Pointer to a boolean indicating if the audio playback is paused.
 */
struct Context
{
    ma_decoder* decoder1{nullptr};
    ma_decoder* decoder2{nullptr};
    bool* is_decoder1_used{nullptr};
    std::atomic<bool>* is_audio_finished{nullptr};
    std::atomic<bool>* is_audio_paused{nullptr};
};

class CoreComponents
{
  public:
    CoreComponents(const CoreComponents&)            = delete;
    CoreComponents& operator=(const CoreComponents&) = delete;
    CoreComponents(CoreComponents&&)                 = delete;
    CoreComponents& operator=(CoreComponents&&)      = delete;

    ma_result play(std::string_view audio);
    void setNextAudio();
    [[nodiscard]] std::optional<const std::string> getAudio() const;
    [[nodiscard]] MusicList& getMusicList() noexcept;
    void lockListInfoMutex();
    void unlockListInfoMutex();
    void setPlayMode(PlayMode mode) noexcept;
    [[nodiscard]] PlayMode getPlayMode() const noexcept;
    void pauseOrResume() noexcept;
    ma_result setVolume(float volume);
    [[nodiscard]] float getCurrentVolume() const noexcept;
    ma_result mute();
    [[nodiscard]] std::string getCurrentPlayingAudio() const noexcept;
    [[nodiscard]] std::optional<const float> getCurrentProgress() const;
    [[nodiscard]] ma_decoder* getCurrentDecoder() const noexcept;
    [[nodiscard]] ma_uint32 getSampleRate() const noexcept;
    [[nodiscard]] bool shouldQuit() const noexcept;
    void quit();

    static CoreComponents& getInstance() noexcept;

  private:
    CoreComponents();
    ~CoreComponents();

    void initMusicList();
    void initPlayMode() noexcept;
    void switchDecoder() noexcept;
    ma_result decodeToWAV(std::string_view input_file, std::string_view output_file);
    ma_result reinitUnusedDecoder(std::string_view filename);
    void removeWAVFile();
    void removeAllWAVFiles();
    void deletePtr();

    ma_device* device_{nullptr};
    ma_decoder* decoder1_{nullptr};
    ma_decoder* decoder2_{nullptr};
    ma_decoder_config decoder_config_{};
    ma_encoder* encoder_{nullptr};
    ma_encoder_config encoder_config_{};

    ma_format format_{ma_format_s16};
    ma_uint32 channels_{2};
    ma_uint32 sample_rate_{44100};

    bool* decoder1_used_{nullptr};
    std::atomic<bool>* audio_finished_{nullptr};
    std::atomic<bool>* audio_paused_{nullptr};

    MusicList* music_list_{nullptr};
    std::mutex list_info_mutex_;
    PlayMode play_mode_;

    std::string current_playing_audio_;
    float current_volume_{1.0f};
    float last_volume_{1.0f};

    std::deque<std::string> cached_wav_files_;

    bool should_quit_{false};
};
