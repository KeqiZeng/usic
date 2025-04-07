#include "thread_signals.h"

void AudioFinishedCallbackSignals::signalAudioFinished() noexcept
{
    audio_finished_.store(true);
    flag_.test_and_set();
    flag_.notify_one();
}

void AudioFinishedCallbackSignals::reset() noexcept
{
    flag_.clear();
    audio_finished_.store(false);
}

void AudioFinishedCallbackSignals::wait(bool predicate) const noexcept
{
    flag_.wait(predicate);
}

bool AudioFinishedCallbackSignals::isAudioFinished() const noexcept
{
    return audio_finished_.load();
}

void AudioFinishedCallbackSignals::quit() noexcept
{
    should_quit_.store(true);
    flag_.test_and_set();
    flag_.notify_one();
}

bool AudioFinishedCallbackSignals::shouldQuit() const noexcept
{
    return should_quit_.load();
}

AudioFinishedCallbackSignals& AudioFinishedCallbackSignals::getInstance() noexcept
{
    static AudioFinishedCallbackSignals instance;
    return instance;
}
