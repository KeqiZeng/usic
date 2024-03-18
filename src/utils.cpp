#include "utils.hpp"

#include "commands.hpp"
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

  ma_result result =
      play(pData->pEngine, pData->pSound, musicToPlay, pData->musicPlaying,
           pData->musicList, pSound, pData->shuffle);
  if (result != MA_SUCCESS) {
    // TODO: log and change the loopFlag in main
    error_log(fmt::format("Failed to play music: {}", musicToPlay));
    std::exit(FATAL_ERROR);
  }
  delete pData;
}

// void sound_init_notification_callback(ma_async_notification* pNotification) {
//   auto* pSoundInitNotification =
//       static_cast<SoundInitNotification*>(pNotification);
//   auto* pMyData = static_cast<MaSound*>(pSoundInitNotification->pMyData);
//   pMyData->isInitialized = true;
//   fmt::print("sound initialized\n");
// }

auto ma_sound_mock_init(ma_engine* pEngine, ma_sound* pSound) -> ma_result {
  ma_sound_config soundConfig = ma_sound_config_init();
  soundConfig.pFilePath = nullptr;
  soundConfig.pDataSource = nullptr;
  soundConfig.pInitialAttachment = nullptr;
  soundConfig.initialAttachmentInputBusIndex = 0;
  soundConfig.channelsIn = 0;
  soundConfig.channelsOut = 0;
  return ma_sound_init_ex(pEngine, &soundConfig, pSound);
}