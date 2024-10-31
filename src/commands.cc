#include "commands.h"

void Progress::init(const std::string& _str, float _percent) {
  this->str = std::move(_str);
  this->percent = _percent;
}

const std::string Progress::make_bar() {
  int n = percent * PROGRESS_BAR_LENGTH;
  std::string bar(PROGRESS_BAR_LENGTH, BAR_ELEMENT);
  bar[n] = CURSOR_ELEMENT;
  return fmt::format("{} {}\n", bar, str);
}

static ma_sound* get_playing_pSound(MaComponents* pMa) {
  ma_sound* pSound = nullptr;
  if (!pMa) {
    log("Invalid pMa in function \"get_playing_pSound\"", LogType::ERROR);
    return pSound;
  }
  if (pMa->pSound_to_play->is_playing() &&
      !pMa->pSound_to_register->is_playing()) {
    pSound = pMa->pSound_to_play->pSound.get();
  } else if (!pMa->pSound_to_play->is_playing() &&
             pMa->pSound_to_register->is_playing()) {
    pSound = pMa->pSound_to_register->pSound.get();
  }
  return pSound;
}

ma_result play(MaComponents* pMa, const std::string& musicToPlay,
               std::string* musicPlaying, MusicList* musicList,
               QuitControl* quitC, Config* config) {
  ma_result result;
  if (!pMa) {
    log("Invalid MaComponents in function \"play\"", LogType::ERROR);
    return MA_ERROR;
  }
  if (!pMa->pSound_to_play->is_initialized() &&
      !pMa->pSound_to_register->is_initialized()) {
    result = play_internal(pMa->pEngine.get(), pMa->pSound_to_play.get(),
                           musicToPlay, musicPlaying, musicList,
                           pMa->pSound_to_register.get(), quitC,
                           config->is_random());
    if (result != MA_SUCCESS) {
      log(fmt::format("Failed to play music: {}, error occured in function "
                      "\"play_internal\"",
                      musicToPlay),
          LogType::ERROR);
    }
  } else {
    play_later(config, musicToPlay, musicList);
    result = play_next(pMa);
    if (result != MA_SUCCESS) {
      log(fmt::format("Failed to play music: {}, error occured in function  "
                      "\"play_next\"",
                      musicToPlay),
          LogType::ERROR);
    }
  }
  return result;
}

void play_later(Config* config, const std::string& music,
                MusicList* musicList) {
  if (!musicList) {
    log("Invalid musicList in play_later", LogType::ERROR);
    return;
  }
  if (config->is_repetitive()) {
    // fmt::print("{} removed: {}\n", music, musicList->remove(music));
    musicList->remove(music);
  }
  musicList->head_in(music);
}

ma_result play_next(MaComponents* pMa) {
  ma_sound* pSound = get_playing_pSound(pMa);
  if (pSound == nullptr) {
    // TODO: log
    log("Failed to get playing pSound", LogType::ERROR);
    return MA_ERROR;
  }

  ma_uint64 length = 0;
  ma_result result = ma_sound_get_length_in_pcm_frames(pSound, &length);
  if (result != MA_SUCCESS) {
    log("Failed to get length in pcm frames", LogType::ERROR);
    return result;
  }
  result = ma_sound_seek_to_pcm_frame(pSound, length);
  if (result != MA_SUCCESS) {
    log("Failed to seek to pcm frame", LogType::ERROR);
    return result;
  }
  if (ma_sound_is_playing(pSound) == 0U) {
    result = ma_sound_start(pSound);
    if (result != MA_SUCCESS) {
      log("Failed to start sound", LogType::ERROR);
      return result;
    }
  }
  return MA_SUCCESS;
}

ma_result play_prev(MaComponents* pMa, MusicList* musicList) {
  if (!musicList || musicList->is_empty()) {
    log("Invalid musicList in play_prev", LogType::ERROR);
    return MA_ERROR;
  }
  auto prevMusic = musicList->tail_out();
  if (prevMusic == nullptr) {
    log("Failed to get prev music", LogType::ERROR);
    return MA_ERROR;
  }
  musicList->head_in(prevMusic->get_music());
  return play_next(pMa);
}

ma_result pause_resume(MaComponents* pMa) {
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

ma_result stop(MaComponents* pMa) {
  ma_sound* pSound = get_playing_pSound(pMa);
  if (pSound == nullptr) {
    // TODO: log
    return MA_ERROR;
  }

  ma_result result;
  result = ma_sound_stop(pSound);
  if (result != MA_SUCCESS) {
    // TODO: log
    return result;
  }
  return MA_SUCCESS;
}

ma_result move_cursor(MaComponents* pMa, int seconds) {
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

ma_result set_cursor(MaComponents* pMa, const std::string& time) {
  ma_sound* pSound = get_playing_pSound(pMa);
  if (pSound == nullptr) {
    // TODO: log
    return MA_ERROR;
  }

  // Convert the time string to seconds
  int destination = utils::timeStr_to_sec(time);
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

ma_result get_current_progress(MaComponents* pMa, Progress* currentProgress) {
  ma_sound* pSound = get_playing_pSound(pMa);
  if (pSound == nullptr) {
    // TODO: log
    return MA_ERROR;
  }

  float cursor = 0.0F;
  float duration = 0.0F;

  // Get the current cursor position
  ma_result result = ma_sound_get_cursor_in_seconds(pSound, &cursor);
  if (result != MA_SUCCESS) {
    return result;
  }

  // Get the duration of the sound
  result = ma_sound_get_length_in_seconds(pSound, &duration);
  if (result != MA_SUCCESS) {
    // TODO: log
    return result;
  }

  cursor = round(cursor);
  duration = round(duration);

  std::string cursorStr = utils::sec_to_timeStr(cursor);
  std::string durationStr = utils::sec_to_timeStr(duration);
  if (cursorStr.empty()) {
    log(fmt::format("Failed to convert cursor position \"{}\" to time string: "
                    "cursor position "
                    "should be >= 0",
                    cursor),
        LogType::ERROR);
    return MA_ERROR;
  }
  if (durationStr.empty()) {
    log(fmt::format(
            "Failed to convert duration \"{}\" to time string: duration "
            "should be >= 0",
            duration),
        LogType::ERROR);
    return MA_ERROR;
  }

  std::string _str = fmt::format("{}/{}", cursorStr, durationStr);
  if (!_str.empty()) {
    currentProgress->init(_str, cursor / duration);
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
ma_result adjust_volume(ma_engine* pEngine, float diff) {
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
ma_result get_volume(ma_engine* pEngine, float* volume) {
  ma_device* pDevice = ma_engine_get_device(pEngine);

  return ma_device_get_master_volume(pDevice, volume);
}

ma_result mute_toggle(ma_engine* pEngine) {
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

void quit(MaComponents* pMa, QuitControl* quitC) {
  if (stop(pMa) != MA_SUCCESS) {
    log("Failed to stop music", LogType::ERROR);
  }
  quitC->quit();
}