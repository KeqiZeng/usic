#pragma once
#include <memory>
#include <mutex>

#include "miniaudio.h"
#include "music_list.h"

#define LOADING_FLAGS MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC

class SoundPack;
struct MaComponents;
struct UserData;
class QuitController;

struct SoundInitNotification
{
    ma_async_notification_callbacks cb_;
    void* data_;
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
    bool initialized_{false};
    bool playing_{false};
    std::mutex mtx_;
    std::unique_ptr<SoundInitNotification> sound_init_notification_{nullptr};

    friend void soundInitNotificationCallback(ma_async_notification* notification);
    friend ma_result playInternal(
        ma_engine* engine,
        SoundPack* sound_to_play,
        std::string_view music_to_play,
        std::string* music_playing,
        MusicList* music_list,
        SoundPack* sound_to_register,
        QuitController* quit_controller,
        bool* random,
        bool* repetitive
    );
};

struct MaComponents
{
    std::unique_ptr<ma_engine> engine_;
    std::unique_ptr<SoundPack> sound_to_play_;
    std::unique_ptr<SoundPack> sound_to_register_;

    MaComponents();
    ~MaComponents();

    void maCompInitEngine();
};

struct UserData
{
    ma_engine* engine_;
    SoundPack* sound_to_play_;
    SoundPack* sound_to_register_;
    QuitController* quit_controller_;
    MusicList* music_list_;
    std::string* music_playing_;
    bool* random_;
    bool* repetitive_;

    UserData(
        ma_engine* engine,
        SoundPack* sound_to_play,
        SoundPack* sound_to_register,
        QuitController* quit_controller,
        MusicList* music_list,
        std::string* music_playing,
        bool* random,
        bool* repetitive
    );
};

class QuitController
{
  public:
    void signal();
    void reset();
    void quit();
    bool atEnd();
    bool getQuitFlag();

  private:
    std::mutex mtx_;
    std::condition_variable cv_;
    bool end_{false};
    bool quit_flag{false};

    friend auto cleanFunc(MaComponents* ma_comp, QuitController* quit_controller) -> void;
};

ma_result playInternal(
    ma_engine* engine,
    SoundPack* sound_to_play,
    std::string_view music_to_play,
    std::string* music_playing,
    MusicList* music_list,
    SoundPack* sound_to_register,
    QuitController* quit_controller,
    bool* random,
    bool* repetitive
);

void cleanFunc(MaComponents* ma_comp, QuitController* quit_controller);
void soundAtEndCallback(void* user_data, ma_sound* sound);
void soundInitNotificationCallback(ma_async_notification* notification);