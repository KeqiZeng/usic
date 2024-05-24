#pragma once
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>

#include "callback.hpp"
#include "core.hpp"
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
  // friend void sound_at_end_callback(void* pUserData, ma_sound* pSound);
  friend auto play_internal(ma_engine* pEngine, SoundPack* pSound_to_play,
                            const std::string& musicToPlay,
                            std::string* musicPlaying, MusicList* musicList,
                            SoundPack* pSound_to_register,
                            SoundFinished* soundFinished,
                            bool shuffle) -> ma_result;

 private:
  bool isInitialized{false};
  bool isPlaying{false};
  std::mutex mtx;
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
    std::lock_guard<std::mutex> lock(this->mtx);
    if (this->isInitialized) {
      log("Can not init sound twice", LogType::ERROR);
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

    return ma_sound_init_ex(pEngine, &config, this->pSound.get());
  }

  auto uninit() -> void {
    if (this->isInitialized && this->pSound) {
      std::lock_guard<std::mutex> lock(this->mtx);
      ma_sound_uninit(this->pSound.get());
      this->isInitialized = false;
      this->isPlaying = false;
      log("uninit sound", LogType::INFO);
    }
  }

  auto is_initialized() -> bool { return this->isInitialized; }
  auto is_playing() -> bool { return this->isPlaying; }
};

class SoundFinished {
  friend auto cleanFunc(MaComponents* pMa,
                        SoundFinished* soundFinished) -> void;
  std::mutex mtx;
  std::condition_variable cv;
  bool finished{false};
  bool quitFlag{false};

 public:
  auto signal() -> void {
    std::lock_guard<std::mutex> lock(this->mtx);
    this->finished = true;
    this->cv.notify_one();
  }

  auto reset() -> void { this->finished = false; }
  auto quit() -> void {
    std::lock_guard<std::mutex> lock(this->mtx);
    this->quitFlag = true;
    this->cv.notify_one();
  }

  auto is_finished() -> bool { return this->finished; }
  auto get_quit_flag() -> bool { return this->quitFlag; }
};

class UserData {
 public:
  ma_engine* pEngine;
  SoundPack* pSound_to_play;
  SoundPack* pSound_to_register;
  SoundFinished* soundFinished;
  MusicList* musicList;
  std::string* musicPlaying;
  bool shuffle;

  UserData(ma_engine* _pEngine, SoundPack* _pSound_to_play,
           SoundPack* _pSound_to_register, SoundFinished* _soundFinished,
           MusicList* _musicList, std::string* _musicPlaying, bool _shuffle)
      : pEngine(_pEngine),
        pSound_to_play(_pSound_to_play),
        pSound_to_register(_pSound_to_register),
        soundFinished(_soundFinished),
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
      log("Failed to initialize engine", LogType::ERROR);
    }
    return result;
  }
};

struct Progress {
  std::string str;
  float percent;
};
