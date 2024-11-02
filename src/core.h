#pragma once
#include <memory>
#include <mutex>

#include "miniaudio.h"
#include "music_list.h"

#define LOADING_FLAGS \
  MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC

class SoundPack;
class MaComponents;
class UserData;
class QuitControl;

struct SoundInitNotification {
  ma_async_notification_callbacks cb;
  void* pMyData;
};

class SoundPack {
  friend void sound_init_notification_callback(
      ma_async_notification* pNotification);
  // friend void sound_at_end_callback(void* pUserData, ma_sound* pSound);
  friend ma_result play_internal(ma_engine* pEngine, SoundPack* pSound_to_play,
                                 const std::string& musicToPlay,
                                 std::string* musicPlaying,
                                 MusicList* musicList,
                                 SoundPack* pSound_to_register,
                                 QuitControl* quitC, bool* random,
                                 bool* repetitive);

 private:
  bool isInitialized{false};
  bool isPlaying{false};
  std::mutex mtx;
  std::unique_ptr<SoundInitNotification> soundInitNotification{nullptr};

 public:
  std::unique_ptr<ma_sound> pSound{nullptr};
  SoundPack();
  ~SoundPack();

  ma_result init(ma_engine* pEngine, const std::string& pFilePath,
                 ma_uint32 flags);
  void uninit();
  bool is_initialized();
  bool is_playing();
};

class MaComponents {
 public:
  std::unique_ptr<ma_engine> pEngine;
  std::unique_ptr<SoundPack> pSound_to_play;
  std::unique_ptr<SoundPack> pSound_to_register;

  MaComponents();
  ~MaComponents();

  void ma_comp_init_engine();
};

class UserData {
 public:
  ma_engine* pEngine;
  SoundPack* pSound_to_play;
  SoundPack* pSound_to_register;
  QuitControl* quitC;
  MusicList* musicList;
  std::string* musicPlaying;
  bool* random;
  bool* repetitive;

  UserData(ma_engine* _pEngine, SoundPack* _pSound_to_play,
           SoundPack* _pSound_to_register, QuitControl* _quitC,
           MusicList* _musicList, std::string* _musicPlaying, bool* _random,
           bool* _repetitive);
};

class QuitControl {
  friend auto cleanFunc(MaComponents* pMa, QuitControl* quitC) -> void;
  std::mutex mtx;
  std::condition_variable cv;
  bool end{false};
  bool quitF{false};

 public:
  void signal();
  void reset();
  void quit();
  bool at_end();
  bool get_quit_flag();
};

ma_result play_internal(ma_engine* pEngine, SoundPack* pSound_to_play,
                        const std::string& musicToPlay,
                        std::string* musicPlaying, MusicList* musicList,
                        SoundPack* pSound_to_register, QuitControl* quitC,
                        bool* random, bool* repetitive);

void cleanFunc(MaComponents* pMa, QuitControl* quitC);
void sound_at_end_callback(void* pUserData, ma_sound* pSound);
void sound_init_notification_callback(ma_async_notification* pNotification);