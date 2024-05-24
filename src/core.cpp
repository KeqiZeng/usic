#include "core.hpp"

#include <mutex>
#include <thread>

#include "commands.hpp"
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

  ma_result result = pSound_to_play->init(pEngine, musicToPlay, LOADING_FLAGS);
  if (result != MA_SUCCESS) {
    log(fmt::format("Failed to initialize sound from file: {}", musicToPlay),
        LogType::ERROR);
    return result;
  }

  if (shuffle) {
    auto music = musicList->random_out();
    if (!music) {
      log(fmt::format("Failed to get random music: {}", musicToPlay),
          LogType::ERROR);
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
    log(fmt::format("Failed to start sound: {}", musicToPlay), LogType::ERROR);
    return result;
  }
  {
    std::lock_guard<std::mutex> lock(pSound_to_play->mtx);
    pSound_to_play->isPlaying = true;
  }
  log(fmt::format("{} started", musicToPlay), LogType::INFO);
  return MA_SUCCESS;
}

auto cleanFunc(MaComponents* pMa, SoundFinished* soundFinished) -> void {
  std::unique_lock<std::mutex> lock(soundFinished->mtx);
  while (true) {
    soundFinished->cv.wait(lock, [soundFinished] {
      return soundFinished->is_finished() || soundFinished->get_quit_flag();
    });
    if (soundFinished->get_quit_flag()) {
      log("Cleaner quit", LogType::INFO);
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
}