#include "commands.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <utility>

#include "components.hpp"
#include "constants.hpp"
#include "fmt/core.h"
#include "runtime.hpp"

/*
 * Convert time(string like mm:ss) to seconds(int).
 */
static auto timeStr_to_sec(const std::string& timeStr) -> int {
  int min = 0;
  int sec = 0;

  std::istringstream iss(timeStr);
  char colon;
  if (!(iss >> min >> colon >> sec) || colon != ':') {
    return -1;  // failed conversion
  }

  // minutes should not be less than 0
  if (min < 0) {
    return -1;
  }

  // seconds should be 0-60
  if (sec < 0 || sec > SECONDS_PER_MINUTE) {
    return -1;
  }

  return min * SECONDS_PER_MINUTE + sec;
}

/*
 * Convert seconds(int) to time(string like mm:ss).
 */
static auto sec_to_timeStr(int seconds) -> std::string {
  if (seconds < 0) {
    error_log(
        fmt::format("Failed to convert seconds \"{}\" to time string: seconds "
                    "should be >= 0",
                    seconds));
    return "";
  }

  int min = seconds / SECONDS_PER_MINUTE;
  int sec = seconds % SECONDS_PER_MINUTE;

  return fmt::format("{:d}:{:02d}", min, sec);
}

auto play(ma_engine* pEngine, SoundPack* pSound_to_play,
          const std::string& musicToPlay, std::string* musicPlaying,
          MusicList* musicList, SoundPack* pSound_to_register,
          bool shuffle) -> ma_result {
  *musicPlaying = musicToPlay;

  pSound_to_register->isPlaying = false;

  pSound_to_play->uninit();
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
                               musicList, musicPlaying, shuffle));

  ma_sound_set_end_callback(pSound_to_play->pSound.get(), sound_at_end_callback,
                            pUserData);
  result = ma_sound_start(pSound_to_play->pSound.get());
  if (result != MA_SUCCESS) {
    // TODO: log
    error_log(fmt::format("Failed to start sound: {}", musicToPlay));
    return result;
  }
  pSound_to_play->isPlaying = true;
  fmt::print("{} started\n", musicToPlay);
  return MA_SUCCESS;
}

auto play_later(const std::string& music, MusicList* musicList) -> void {
  if (!musicList) {
    error_log("Invalid musicList in play_later");
    return;
  }
  if (musicList->contain(music)) {
    musicList->remove(music);
  }
  musicList->head_in(music);
}

static auto get_playing_pSound(MaComponents* pMa) -> ma_sound* {
  ma_sound* pSound = nullptr;
  if (!pMa) {
    error_log("Invalid pMa in get_playing_pSound");
    return pSound;
  }
  if (pMa->pSound_to_play && !pMa->pSound_to_register) {
    if (pMa->pSound_to_play->is_playing()) {
      pSound = pMa->pSound_to_play->pSound.get();
    }
  } else if (!pMa->pSound_to_play && pMa->pSound_to_register) {
    if (pMa->pSound_to_register->is_playing()) {
      pSound = pMa->pSound_to_register->pSound.get();
    }
  } else if (pMa->pSound_to_play && pMa->pSound_to_register) {
    if (pMa->pSound_to_play->is_playing() &&
        !pMa->pSound_to_register->is_playing()) {
      pSound = pMa->pSound_to_play->pSound.get();
    } else if (!pMa->pSound_to_play->is_playing() &&
               pMa->pSound_to_register->is_playing()) {
      pSound = pMa->pSound_to_register->pSound.get();
    }
  }
  return pSound;
}

auto play_next(MaComponents* pMa) -> ma_result {
  ma_sound* pSound = get_playing_pSound(pMa);
  if (pSound == nullptr) {
    // TODO: log
    error_log("Failed to get playing pSound");
    return MA_ERROR;
  }

  ma_uint64 length = 0;
  ma_result result = ma_sound_get_length_in_pcm_frames(pSound, &length);
  if (result != MA_SUCCESS) {
    error_log("Failed to get length in pcm frames");
    return result;
  }
  result = ma_sound_seek_to_pcm_frame(pSound, length);
  if (result != MA_SUCCESS) {
    error_log("Failed to seek to pcm frame");
    return result;
  }
  return MA_SUCCESS;
}

auto play_prev(MaComponents* pMa, MusicList* musicList) -> ma_result {
  if (!musicList || musicList->is_empty()) {
    error_log("Invalid musicList in play_prev");
    return MA_ERROR;
  }
  auto prevMusic = musicList->tail_out();
  if (prevMusic == nullptr) {
    error_log("Failed to get prev music");
    return MA_ERROR;
  }
  musicList->head_in(prevMusic->get_music());
  return play_next(pMa);
}

auto pause_resume(MaComponents* pMa) -> ma_result {
  ma_sound* pSound = get_playing_pSound(pMa);
  if (pSound == nullptr) {
    // TODO: log
    return MA_ERROR;
  }

  ma_result result;
  if (ma_sound_is_playing(pSound) != 0U) {
    // if is_playing, pause
    result = ma_sound_stop(pSound);
    if (result != MA_SUCCESS) {
      // TODO: log
      return result;
    }
  } else {
    // else resume
    result = ma_sound_start(pSound);
    if (result != MA_SUCCESS) {
      // TODO: log
      return result;
    }
  }
  return MA_SUCCESS;
}

auto move_cursor(MaComponents* pMa, int seconds) -> ma_result {
  ma_sound* pSound = get_playing_pSound(pMa);
  if (pSound == nullptr) {
    // TODO: log
    return MA_ERROR;
  }

  // Get current position of cursor and total length
  ma_uint64 cursor = 0;
  ma_uint64 length = 0;
  ma_result result = ma_sound_get_cursor_in_pcm_frames(pSound, &cursor);
  if (result != MA_SUCCESS) {
    // TODO: log
    return result;
  }
  result = ma_sound_get_length_in_pcm_frames(pSound, &length);
  if (result != MA_SUCCESS) {
    // TODO: log
    return result;
  }

  // Calculate the position of the cursor after moving
  ma_uint64 movedCursor = 0;
  if (seconds > 0) {
    int newCursor =
        cursor + (ma_engine_get_sample_rate(pMa->pEngine.get()) * seconds);
    movedCursor = newCursor < length ? newCursor : length;
  } else {
    seconds = -seconds;
    int new_cursor =
        cursor - (ma_engine_get_sample_rate(pMa->pEngine.get()) * seconds);
    movedCursor = new_cursor > 0 ? new_cursor : 0;
  }

  // Move cursor
  result = ma_sound_seek_to_pcm_frame(pSound, movedCursor);
  if (result != MA_SUCCESS) {
    // TODO: log
    return result;
  }
  return result;
}

auto set_cursor(MaComponents* pMa, const std::string& time) -> ma_result {
  ma_sound* pSound = get_playing_pSound(pMa);
  if (pSound == nullptr) {
    // TODO: log
    return MA_ERROR;
  }

  // Convert the time string to seconds
  int destination = timeStr_to_sec(time);
  if (destination == -1) {
    return MA_INVALID_ARGS;
  }

  // Get the total length of the sound
  float length = 0;
  ma_result result = ma_sound_get_length_in_seconds(pSound, &length);
  if (result != MA_SUCCESS) {
    return result;
  }

  int lenInt = static_cast<int>(length);  // must be rounded down

  // Calculate the destination
  destination = destination < 0        ? 0
                : destination > lenInt ? lenInt
                                       : destination;

  result = ma_sound_seek_to_pcm_frame(
      pSound, (ma_engine_get_sample_rate(pMa->pEngine.get()) * destination));

  if (result != MA_SUCCESS) {
    // TODO: log
    return result;
  }
  return result;
}

auto get_current_progress(MaComponents* pMa,
                          Progress* currentProgress) -> ma_result {
  ma_sound* pSound = get_playing_pSound(pMa);
  if (pSound == nullptr) {
    // TODO: log
    return MA_ERROR;
  }

  float cursor = 0.0F;
  float length = 0.0F;

  // Get the current cursor position
  ma_result result = ma_sound_get_cursor_in_seconds(pSound, &cursor);
  if (result != MA_SUCCESS) {
    return result;
  }

  // Get the total length of the sound
  result = ma_sound_get_length_in_seconds(pSound, &length);
  if (result != MA_SUCCESS) {
    // TODO: log
    return result;
  }

  cursor = round(cursor);
  length = round(length);

  std::string cursorStr = sec_to_timeStr(cursor);
  std::string lengthStr = sec_to_timeStr(length);
  if (cursorStr.empty() || lengthStr.empty()) {
    // TODO: log
    return MA_ERROR;
  }

  std::string _str = fmt::format("{}/{}", cursorStr, lengthStr);
  if (!_str.empty()) {
    currentProgress->str = std::move(_str);
    currentProgress->percent = cursor / length;
  } else {
    // TODO: log
    return MA_ERROR;
  }
  return result;
}

/*
 * Adjust the volume of the engine, which should be 0.0-1.0.
 * The parameter "diff" indicates the step length.
 */
auto adjust_volume(ma_engine* pEngine, float diff) -> ma_result {
  ma_device* pDevice = ma_engine_get_device(pEngine);

  // Get current volume of the engine
  float currentVolume = 0.0;
  ma_result result = ma_device_get_master_volume(pDevice, &currentVolume);
  if (result != MA_SUCCESS) {
    // TODO: log
    return result;
  }

  // Calculate the new volume
  float newVolume = currentVolume + diff;
  newVolume = newVolume < 0.0F ? 0.0F : newVolume > 1.0F ? 1.0F : newVolume;

  // Set volume
  result = ma_device_set_master_volume(pDevice, newVolume);
  if (result != MA_SUCCESS) {
    // TODO: log
    return result;
  }
  return MA_SUCCESS;
}

/*
 * Get the volume of the engine and print it as 0-100%
 */
auto get_volume(ma_engine* pEngine, float* volume) -> ma_result {
  ma_device* pDevice = ma_engine_get_device(pEngine);

  return ma_device_get_master_volume(pDevice, volume);
}

auto mute_toggle(ma_engine* pEngine) -> ma_result {
  ma_device* pDevice = ma_engine_get_device(pEngine);

  float currentVolume = 0.0;
  ma_result result = ma_device_get_master_volume(pDevice, &currentVolume);
  if (result != MA_SUCCESS) {
    // TODO: log
    return result;
  }
  if (currentVolume == 0.0F) {
    result = ma_device_set_master_volume(pDevice, 1.0F);
    if (result != MA_SUCCESS) {
      // TODO: log
      return result;
    }
  } else {
    result = ma_device_set_master_volume(pDevice, 0.0F);
    if (result != MA_SUCCESS) {
      // TODO: log
      return result;
    }
  }
  return MA_SUCCESS;
}