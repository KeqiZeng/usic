#pragma once

#include <atomic>

class AudioFinishedCallbackSignals
{
  public:
    AudioFinishedCallbackSignals(AudioFinishedCallbackSignals&)             = delete;
    AudioFinishedCallbackSignals& operator=(AudioFinishedCallbackSignals&)  = delete;
    AudioFinishedCallbackSignals(AudioFinishedCallbackSignals&&)            = delete;
    AudioFinishedCallbackSignals& operator=(AudioFinishedCallbackSignals&&) = delete;

    void signal() noexcept;
    void reset() noexcept;
    void wait(bool predicate) const noexcept;
    bool isAudioFinished() const noexcept;
    void quit() noexcept;
    bool shouldQuit() const noexcept;

    static AudioFinishedCallbackSignals& getInstance() noexcept;

  private:
    AudioFinishedCallbackSignals()  = default;
    ~AudioFinishedCallbackSignals() = default;

    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
    std::atomic<bool> audio_finished_{false};
    std::atomic<bool> should_quit_{false};
};