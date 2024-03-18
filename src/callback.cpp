#include "callback.hpp"

#include "components.hpp"
#include "constants.hpp"

void sound_at_end_callback(void* pUserData, ma_sound* pSound) {
  auto* pData = static_cast<UserData*>(pUserData);
  const std::string& musicPlaying = *(pData->musicPlaying);
  pData->musicList->tail_in(musicPlaying);

  std::string musicToPlay;
  if (pData->shuffle) {
    musicToPlay = pData->musicList->random_out()->get_music();
  } else {
    musicToPlay = pData->musicList->head_out()->get_music();
  }

  ma_result result = play(pData->pEngine, pData->pSound_to_play, musicToPlay,
                          pData->musicPlaying, pData->musicList,
                          pData->pSound_to_register, pData->shuffle);
  if (result != MA_SUCCESS) {
    // TODO: log and change the loopFlag in main
    error_log(fmt::format("Failed to play music: {}", musicToPlay));
    std::exit(FATAL_ERROR);
  }
  delete pData;
}

void sound_init_notification_callback(ma_async_notification* pNotification) {
  auto* pSoundInitNotification =
      static_cast<SoundInitNotification*>(pNotification);
  auto* pMyData = static_cast<SoundPack*>(pSoundInitNotification->pMyData);
  pMyData->isInitialized = true;
  fmt::print("sound initialized\n");
}