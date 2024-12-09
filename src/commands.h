#pragma once
#include <string>

#include "core.h"
#include "miniaudio.h"
#include "music_list.h"
#include "runtime.h"

class Progress
{
  public:
    void init(std::string_view music_name, std::string_view progress, float percent);
    const std::string makeBar();

  private:
    std::string music_name_;
    std::string progress_;
    float percent_;
};

namespace commands
{

void load(std::string_view list_name, MusicList* music_list, Config* config);
ma_result play(
    MaComponents* ma_comp,
    std::string_view music_to_play,
    std::string* music_playing,
    MusicList* music_list,
    Controller* controller,
    Config* config
);

void playLater(std::string_view music, MusicList* music_list, Config* config);
ma_result playNext(MaComponents* ma_comp, MusicList* music_list);
ma_result playPrev(MaComponents* ma_comp, MusicList* music_list);
ma_result pause(MaComponents* ma_comp);
ma_result seekForward(MaComponents* ma_comp);
ma_result seekBackward(MaComponents* ma_comp);
ma_result seekTo(MaComponents* ma_comp, std::string_view time);
ma_result getProgress(MaComponents* ma_comp, const std::string& music_playing, Progress* current_progress);
ma_result volumeUp(EnginePack* engine);
ma_result volumeDown(EnginePack* engine);
ma_result setVolume(EnginePack* engine, std::string_view volume_str);
float getVolume(EnginePack* engine);
ma_result mute(EnginePack* engine);
void setMode(PlayMode mode, MusicList* music_list, Config* config);
PlayMode getMode(Config* config);
std::vector<std::string> getList(MusicList* music_list);
void quit(MaComponents* ma_comp, Controller* controller);

} // namespace commands