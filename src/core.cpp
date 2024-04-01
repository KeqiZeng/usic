#include "core.hpp"

#include <mutex>

#include "components.hpp"
#include "constants.hpp"
#include "runtime.hpp"

auto play_internal(ma_engine* pEngine, SoundPack* pSound_to_play,
                   const std::string& musicToPlay, std::string* musicPlaying,
                   MusicList* musicList, SoundPack* pSound_to_register,
                   SoundFinished* soundFinished, bool shuffle) -> ma_result {
  *musicPlaying = musicToPlay;

  {
    std::lock_guard<std::mutex> lock(pSound_to_register->mtx);
    pSound_to_register->isPlaying = false;
  }

  // pSound_to_play->uninit();
  ma_result result = pSound_to_play->init(pEngine, musicToPlay, LOADING_FLAGS);
  if (result != MA_SUCCESS) {
    error_log(
        fmt::format("Failed to initialize sound from file: {}", musicToPlay));
    return result;
  }

  if (shuffle) {
    auto music = musicList->random_out();
    if (!music) {
      error_log(fmt::format("Failed to get random music: {}", musicToPlay));
      return MA_ERROR;
    }
    musicList->head_in(music->get_music());
  }

  auto* pUserData(new UserData(pEngine, pSound_to_register, pSound_to_play,
                               soundFinished, musicList, musicPlaying,
                               shuffle));

  ma_sound_set_end_callback(pSound_to_play->pSound.get(), sound_at_end_callback,
                            pUserData);
  result = ma_sound_start(pSound_to_play->pSound.get());
  if (result != MA_SUCCESS) {
    error_log(fmt::format("Failed to start sound: {}", musicToPlay));
    return result;
  }
  {
    std::lock_guard<std::mutex> lock(pSound_to_play->mtx);
    pSound_to_play->isPlaying = true;
  }
  fmt::print("{} started\n", musicToPlay);
  return MA_SUCCESS;
}

auto cleanFunc(MaComponents* pMa, SoundFinished* soundFinished) -> void {
  std::unique_lock<std::mutex> lock(soundFinished->mtx);
  while (true) {
    soundFinished->cv.wait(lock, [soundFinished] {
      return soundFinished->finished || soundFinished->quitFlag;
    });
    if (soundFinished->quitFlag) {
      return;
    }
    if (!pMa->pSound_to_play->is_playing() &&
        pMa->pSound_to_play->is_initialized()) {
      pMa->pSound_to_play->uninit();
    }
    if (!pMa->pSound_to_register->is_playing() &&
        pMa->pSound_to_register->is_initialized()) {
      pMa->pSound_to_register->uninit();
    }
    soundFinished->reset();
  }
}