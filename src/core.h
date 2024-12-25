#pragma once

#include "miniaudio.h"
#include <atomic>
#include <string>
#include <string_view>

void dataCallback(ma_device* device, void* output, const void* input, ma_uint32 frame_count);
void audioFinishedCallback();

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

    ma_result play(std::string_view filename);
    ma_result moveCursorToStart();
    ma_result moveCursorToEnd();
    [[nodiscard]] std::string getCurrentPlayingAudio() const noexcept;
    void pauseOrResume() noexcept;
    void clearWAVFile();

    static CoreComponents& getInstance() noexcept;

  private:
    CoreComponents();
    ~CoreComponents();

    void switchDecoder() noexcept;
    ma_result decodeToWAV(std::string_view input_file, std::string_view output_file);
    ma_result reinitUnusedDecoder(std::string_view filename);
    void deletePtr();

    ma_device* device_{nullptr};
    ma_decoder* decoder1_{nullptr};
    ma_decoder* decoder2_{nullptr};
    ma_decoder_config decoder_config_{};
    ma_encoder* encoder_{nullptr};
    ma_encoder_config encoder_config_{};

    bool* decoder1_used_{nullptr};
    std::atomic<bool>* audio_finished_{nullptr};
    std::atomic<bool>* audio_paused_{nullptr};

    std::string current_playing_audio_;

    ma_format format_{ma_format_s16};
    ma_uint32 channels_{2};
    ma_uint32 sample_rate_{44100};
};