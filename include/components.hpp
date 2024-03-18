#pragma once
#include <memory>
#include <string>

#include "callback.hpp"
#include "commands.hpp"
#include "miniaudio.h"
#include "music_list.hpp"

struct SoundInitNotification {
  ma_async_notification_callbacks cb;
  void* pMyData;

  SoundInitNotification() = default;
};

class SoundPack {
  friend void sound_init_notification_callback(
      ma_async_notification* pNotification);
  friend auto play(ma_engine* pEngine, SoundPack* pSound_to_play,
                   const std::string& musicToPlay, std::string* musicPlaying,
                   MusicList* musicList, SoundPack* pSound_to_register,
                   bool shuffle) -> ma_result;

 private:
  bool isInitialized{false};
  bool isPlaying{false};
  std::unique_ptr<SoundInitNotification> soundInitNotification{nullptr};

 public:
  std::unique_ptr<ma_sound> pSound{nullptr};
  SoundPack() : soundInitNotification(new SoundInitNotification) {
    this->soundInitNotification->cb.onSignal = sound_init_notification_callback;
    this->soundInitNotification->pMyData = this;
  }
  ~SoundPack() { this->uninit(); }

  auto init(ma_engine* pEngine, const std::string& pFilePath,
            ma_uint32 flags) -> ma_result {
    if (this->isInitialized) {
      error_log("Can not init sound twice");
      return MA_ERROR;
    }
    if (!this->pSound) {
      this->pSound = std::unique_ptr<ma_sound>(new ma_sound);
    }
    ma_sound_config config = ma_sound_config_init_2(pEngine);
    config.pFilePath = pFilePath.c_str();
    config.flags = flags;
    config.initNotifications.done.pNotification =
        this->soundInitNotification.get();

    ma_result result = ma_sound_init_ex(pEngine, &config, this->pSound.get());
    if (result != MA_SUCCESS) {
      error_log(
          fmt::format("Failed to initialize sound from file: {}", pFilePath));
    }
    return result;
  }

  auto uninit() -> void {
    if (this->isInitialized && this->pSound) {
      ma_sound_uninit(this->pSound.get());
      this->isInitialized = false;
      fmt::print("uninit sound\n");
    }
  }

  auto is_initialized() -> bool { return this->isInitialized; }
  auto is_playing() -> bool { return this->isPlaying; }
};

class UserData {
 public:
  ma_engine* pEngine;
  SoundPack* pSound_to_play;
  SoundPack* pSound_to_register;
  MusicList* musicList;
  std::string* musicPlaying;
  bool shuffle;

  UserData(ma_engine* _pEngine, SoundPack* _pSound_to_play,
           SoundPack* _pSound_to_register, MusicList* _musicList,
           std::string* _musicPlaying, bool _shuffle)
      : pEngine(_pEngine),
        pSound_to_play(_pSound_to_play),
        pSound_to_register(_pSound_to_register),
        musicList(_musicList),
        musicPlaying(_musicPlaying),
        shuffle(_shuffle) {}
  ~UserData() = default;
};

class MaComponents {
 public:
  std::unique_ptr<ma_engine> pEngine;
  std::unique_ptr<SoundPack> pSound_to_play;
  std::unique_ptr<SoundPack> pSound_to_register;

  MaComponents()
      : pEngine(new ma_engine),
        pSound_to_play(new SoundPack),
        pSound_to_register(new SoundPack) {}

  ~MaComponents() { ma_engine_uninit(pEngine.get()); }

  auto ma_comp_init_engine() -> ma_result {
    ma_result result = ma_engine_init(nullptr, pEngine.get());
    if (result != MA_SUCCESS) {
      error_log("Failed to initialize engine");
    }
    return result;
  }
};

struct Progress {
  std::string str;
  float percent;
};