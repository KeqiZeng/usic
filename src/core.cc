#include "core.h"

SoundPack::SoundPack() : soundInitNotification(new SoundInitNotification) {
  this->soundInitNotification->cb.onSignal = sound_init_notification_callback;
  this->soundInitNotification->pMyData = this;
}
SoundPack::~SoundPack() { this->uninit(); }

ma_result SoundPack::init(ma_engine* pEngine, const std::string& pFilePath,
                          ma_uint32 flags) {
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

void SoundPack::uninit() {
  if (this->isInitialized && this->pSound) {
    std::lock_guard<std::mutex> lock(this->mtx);
    ma_sound_uninit(this->pSound.get());
    this->isInitialized = false;
    this->isPlaying = false;
    log("uninit sound", LogType::INFO);
  }
}

bool SoundPack::is_initialized() { return this->isInitialized; }
bool SoundPack::is_playing() { return this->isPlaying; }

MaComponents::MaComponents()
    : pEngine(new ma_engine),
      pSound_to_play(new SoundPack),
      pSound_to_register(new SoundPack) {}

MaComponents::~MaComponents() { ma_engine_uninit(pEngine.get()); }

ma_result MaComponents::ma_comp_init_engine() {
  ma_result result = ma_engine_init(nullptr, pEngine.get());
  if (result != MA_SUCCESS) {
    log("Failed to initialize engine", LogType::ERROR);
  }
  return result;
}

UserData::UserData(ma_engine* _pEngine, SoundPack* _pSound_to_play,
                   SoundPack* _pSound_to_register, QuitControl* _quitC,
                   MusicList* _musicList, std::string* _musicPlaying,
                   bool _random)
    : pEngine(_pEngine),
      pSound_to_play(_pSound_to_play),
      pSound_to_register(_pSound_to_register),
      quitC(_quitC),
      musicList(_musicList),
      musicPlaying(_musicPlaying),
      random(_random) {}

void QuitControl::signal() {
  std::lock_guard<std::mutex> lock(this->mtx);
  this->end = true;
  this->cv.notify_one();
}

void QuitControl::reset() { this->end = false; }
void QuitControl::quit() {
  std::lock_guard<std::mutex> lock(this->mtx);
  this->quitF = true;
  this->cv.notify_one();
}

bool QuitControl::at_end() { return this->end; }
bool QuitControl::get_quit_flag() { return this->quitF; }

ma_result play_internal(ma_engine* pEngine, SoundPack* pSound_to_play,
                        const std::string& musicToPlay,
                        std::string* musicPlaying, MusicList* musicList,
                        SoundPack* pSound_to_register, QuitControl* quitC,
                        bool random) {
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

  if (random) {
    auto music = musicList->random_out();
    if (!music) {
      log(fmt::format("Failed to get random music: {}", musicToPlay),
          LogType::ERROR);
      return MA_ERROR;
    }
    musicList->head_in(music->get_music());
  }

  auto* pUserData(new UserData(pEngine, pSound_to_register, pSound_to_play,
                               quitC, musicList, musicPlaying, random));

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

void cleanFunc(MaComponents* pMa, QuitControl* quitC) {
  std::unique_lock<std::mutex> lock(quitC->mtx);
  while (true) {
    quitC->cv.wait(
        lock, [quitC] { return quitC->at_end() || quitC->get_quit_flag(); });
    if (quitC->get_quit_flag()) {
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
    quitC->reset();
  }
}

void sound_at_end_callback(void* pUserData, ma_sound* pSound) {
  auto* pData = static_cast<UserData*>(pUserData);
  if (!pData) {
    log("Invalid user data in sound_at_end_callback", LogType::ERROR);
    std::exit(FATAL_ERROR);
  }
  const std::string& musicPlaying = *(pData->musicPlaying);
  pData->musicList->tail_in(musicPlaying);

  const std::string musicToPlay = pData->musicList->head_out()->get_music();

  ma_result result = play_internal(
      pData->pEngine, pData->pSound_to_play, musicToPlay, pData->musicPlaying,
      pData->musicList, pData->pSound_to_register, pData->quitC, pData->random);
  if (result != MA_SUCCESS) {
    log(fmt::format("Failed to play music: {}", musicToPlay), LogType::ERROR);
    std::exit(FATAL_ERROR);
  }
  pData->quitC->signal();
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