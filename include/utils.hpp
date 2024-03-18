#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "fmt/core.h"
#include "miniaudio.h"
#include "music_list.hpp"
#include "runtime.hpp"

void sound_at_end_callback(void* pUserData, ma_sound* pSound);
// void sound_init_notification_callback(ma_async_notification* pNotification);
auto ma_sound_mock_init(ma_engine* pEngine, ma_sound* pSound) -> ma_result;

class UserData {
 public:
  ma_engine* pEngine;
  ma_sound* pSound;
  MusicList* musicList;
  std::string* musicPlaying;
  bool shuffle;

  UserData(ma_engine* _pEngine, ma_sound* _pSound, MusicList* _musicList,
           std::string* _musicPlaying, bool _shuffle)
      : pEngine(_pEngine),
        pSound(_pSound),
        musicList(_musicList),
        musicPlaying(_musicPlaying),
        shuffle(_shuffle) {}
  ~UserData() = default;
};

// struct SoundInitNotification {
//   ma_async_notification_callbacks cb;
//   void* pMyData;

//   SoundInitNotification() = default;
// };

// class MaSound {
//   friend void sound_init_notification_callback(
//       ma_async_notification* pNotification);
//   friend void sound_at_end_callback(void* pUserData, ma_sound* pSound);

//  private:
//   bool isInitialized{false};
//   std::unique_ptr<SoundInitNotification> soundInitNotification{nullptr};

//  public:
//   std::unique_ptr<ma_sound> pSound{nullptr};
//   MaSound() : soundInitNotification(new SoundInitNotification) {
//     this->soundInitNotification->cb.onSignal =
//     sound_init_notification_callback; this->soundInitNotification->pMyData =
//     this;
//   }
//   ~MaSound() { this->uninit(); }

//   auto init(ma_engine* pEngine, const std::string& pFilePath,
//             ma_uint32 flags) -> ma_result {
//     if (this->isInitialized) {
//       error_log("Can not init sound twice");
//       return MA_ERROR;
//     }
//     if (!this->pSound) {
//       this->pSound = std::unique_ptr<ma_sound>(new ma_sound);
//     }
//     ma_sound_config config = ma_sound_config_init_2(pEngine);
//     config.pFilePath = pFilePath.c_str();
//     config.flags = flags;
//     config.initNotifications.done.pNotification =
//         this->soundInitNotification.get();

//     ma_result result = ma_sound_init_ex(pEngine, &config,
//     this->pSound.get()); if (result != MA_SUCCESS) {
//       error_log(
//           fmt::format("Failed to initialize sound from file: {}",
//           pFilePath));
//     }
//     return result;
//   }

//   auto uninit() -> void {
//     if (this->isInitialized) {
//       ma_sound_uninit(this->pSound.get());
//       this->isInitialized = false;
//     }
//   }

//   auto is_initialized() -> bool { return this->isInitialized; }
// };

// class UserData {
//  public:
//   ma_engine* pEngine;
//   MaSound* pSound_to_play;
//   MaSound* pSound_to_register;
//   MusicList* musicList;
//   std::string* musicPlaying;
//   bool shuffle;

//   UserData(ma_engine* _pEngine, MaSound* _pSound_to_play,
//            MaSound* _pSound_to_register, MusicList* _musicList,
//            std::string* _musicPlaying, bool _shuffle)
//       : pEngine(_pEngine),
//         pSound_to_play(_pSound_to_play),
//         pSound_to_register(_pSound_to_register),
//         musicList(_musicList),
//         musicPlaying(_musicPlaying),
//         shuffle(_shuffle) {}
//   ~UserData() = default;
// };

// class MaComponents {
//  public:
//   std::unique_ptr<ma_engine> pEngine;
//   std::unique_ptr<MaSound> pSound_to_play;
//   std::unique_ptr<MaSound> pSound_to_register;

//   MaComponents()
//       : pEngine(new ma_engine),
//         pSound_to_play(new MaSound),
//         pSound_to_register(new MaSound) {}

//   ~MaComponents() {
//     ma_engine_uninit(pEngine.get());
//     // fmt::print("uninit pEngine\n");
//     // pSound_to_play->uninit();
//     // fmt::print("uninit pSound_to_play\n");
//     // pSound_to_register->uninit();
//     // fmt::print("uninit pSound_to_register\n");
//   }

//   auto ma_comp_init_engine() -> ma_result {
//     ma_result result = ma_engine_init(nullptr, pEngine.get());
//     if (result != MA_SUCCESS) {
//       error_log("Failed to initialize engine");
//     }
//     return result;
//   }
// };

class MaComponents {
 public:
  std::unique_ptr<ma_engine> pEngine;
  std::unique_ptr<ma_sound> pSound_to_play;
  std::unique_ptr<ma_sound> pSound_to_register;

  MaComponents()
      : pEngine(new ma_engine),
        pSound_to_play(new ma_sound),
        pSound_to_register(new ma_sound) {}
  ~MaComponents() {
    ma_engine_uninit(pEngine.get());
    fmt::print("uninit pEngine\n");
    ma_sound_uninit(pSound_to_play.get());
    fmt::print("uninit pSound_to_play\n");
    ma_sound_uninit(pSound_to_register.get());
    fmt::print("uninit pSound_to_register\n");
  }

  auto ma_comp_init() -> ma_result {
    ma_result result = ma_engine_init(nullptr, this->pEngine.get());
    if (result != MA_SUCCESS) {
      error_log("Failed to initialize engine");
      return result;
    }
    result =
        ma_sound_mock_init(this->pEngine.get(), this->pSound_to_play.get());
    if (result != MA_SUCCESS) {
      error_log("Failed to mockingly initialize sound_to_play");
      return result;
    }
    result =
        ma_sound_mock_init(this->pEngine.get(), this->pSound_to_register.get());
    if (result != MA_SUCCESS) {
      error_log("Failed to mockingly initialize sound_to_register");
      return result;
    }
    return MA_SUCCESS;
  }
};
