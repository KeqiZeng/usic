#include "callback.hpp"

#include <thread>

#include "components.hpp"
#include "constants.hpp"
#include "core.hpp"

void sound_at_end_callback(void* pUserData, ma_sound* pSound) {
  auto* pData = static_cast<UserData*>(pUserData);
  if (!pData) {
    log("Invalid user data in sound_at_end_callback", LogType::ERROR);
    std::exit(FATAL_ERROR);
  }
  const std::string& musicPlaying = *(pData->musicPlaying);
  pData->musicList->tail_in(musicPlaying);

  const std::string musicToPlay = pData->musicList->head_out()->get_music();

  ma_result result =
      play_internal(pData->pEngine, pData->pSound_to_play, musicToPlay,
                    pData->musicPlaying, pData->musicList,
                    pData->pSound_to_register, pData->endFlag, pData->shuffle);
  if (result != MA_SUCCESS) {
    // TODO: log and change the loopFlag in main
    log(fmt::format("Failed to play music: {}", musicToPlay), LogType::ERROR);
    std::exit(FATAL_ERROR);
  }
  pData->endFlag->signal();
  delete pData;
}

void sound_init_notification_callback(ma_async_notification* pNotification) {
  auto* pSoundInitNotification =
      static_cast<SoundInitNotification*>(pNotification);
  auto* pMyData = static_cast<SoundPack*>(pSoundInitNotification->pMyData);
  if (!pMyData) {
    log("Invalid user data in sound_init_notification_callback",
        LogType::ERROR);
    std::exit(FATAL_ERROR);
  }
  pMyData->isInitialized = true;
  log("Sound initialized", LogType::INFO);
}
