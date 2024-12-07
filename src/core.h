#pragma once
#include <atomic>
#include <memory>
#include <mutex>

#include "miniaudio.h"
#include "music_list.h"
#include "runtime.h"

#define LOADING_FLAGS MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC

class SoundPack;
struct MaComponents;
struct UserData;
class Controller;

struct SoundInitNotification
{
    ma_async_notification_callbacks cb_;
    void* data_;
};

class EnginePack
{
  public:
    ma_engine* engine_{nullptr};
    EnginePack();
    ~EnginePack();
    float getVolume();
    ma_result setVolume(float volume);
    float getLastVolume();

  private:
    float volume_{1.0F};
    float last_volume_{1.0F};
};

class SoundPack
{
  public:
    std::unique_ptr<ma_sound> sound_{nullptr};
    SoundPack();
    ~SoundPack();

    ma_result init(ma_engine* engine, std::string_view file_path, ma_uint32 flags);
    void uninit();
    bool isInitialized();
    bool isPlaying();

  private:
    std::atomic<bool> initialized_{false};
    std::atomic<bool> playing_{false};
    std::mutex mtx_;
    std::unique_ptr<SoundInitNotification> sound_init_notification_{nullptr};

    friend void soundInitNotificationCallback(ma_async_notification* notification);
    friend ma_result playInternal(
        EnginePack* engine_pack,
        SoundPack* sound_to_play,
        std::string_view music_to_play,
        std::string* music_playing,
        MusicList* music_list,
        SoundPack* sound_to_register,
        Controller* controller,
        Config* config
    );
};

struct MaComponents
{
    std::unique_ptr<EnginePack> engine_pack_;
    std::unique_ptr<SoundPack> sound_to_play_;
    std::unique_ptr<SoundPack> sound_to_register_;

    MaComponents();
    ~MaComponents();
};

struct UserData
{
    EnginePack* engine_pack_;
    SoundPack* sound_to_play_;
    SoundPack* sound_to_register_;
    Controller* controller_;
    MusicList* music_list_;
    std::string* music_playing_;
    Config* config_;

    UserData(
        EnginePack* engine_pack,
        SoundPack* sound_to_play,
        SoundPack* sound_to_register,
        Controller* controller,
        MusicList* music_list,
        std::string* music_playing,
        Config* config
    );
};

class Controller
{
  public:
    void signalEnd();
    void signalError();
    void quit();
    bool atEnd();
    bool getError();
    bool getQuitFlag();
    void waitForCleanerExit();

  private:
    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
    std::atomic<bool> end_{false};
    std::atomic<bool> error_{false};
    std::atomic<bool> quit_flag_{false};
    std::atomic<bool> cleaner_exited_{false};

    friend void cleanFunc(MaComponents* ma_comp, Controller* controller);
};

ma_result playInternal(
    EnginePack* engine_pack,
    SoundPack* sound_to_play,
    std::string_view music_to_play,
    std::string* music_playing,
    MusicList* music_list,
    SoundPack* sound_to_register,
    Controller* controller,
    Config* config
);

void cleanFunc(MaComponents* ma_comp, Controller* controller);
void soundAtEndCallback(void* user_data, ma_sound* sound);
void soundInitNotificationCallback(ma_async_notification* notification);