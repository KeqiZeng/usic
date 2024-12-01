#pragma once
#include <string>

#include "core.h"
#include "miniaudio.h"
#include "music_list.h"

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

void playLater(Config* config, std::string_view music, MusicList* music_list);
ma_result playNext(MaComponents* ma_comp);
ma_result playPrev(MaComponents* ma_comp, MusicList* music_list);
ma_result pause(MaComponents* ma_comp);
ma_result cursorForward(MaComponents* ma_comp);
ma_result cursorBackward(MaComponents* ma_comp);
ma_result setCursor(MaComponents* ma_comp, std::string_view time);
ma_result getProgress(MaComponents* ma_comp, const std::string& music_playing, Progress* current_progress);
ma_result volumeUp(ma_engine* engine);
ma_result volumeDown(ma_engine* engine);
ma_result setVolume(ma_engine* engine, std::string_view volume_str);
ma_result getVolume(ma_engine* engine, float* volume);
ma_result mute(ma_engine* engine);
void setRandom(Config* config);
void setRepetitive(Config* config);
std::vector<std::string> getList(MusicList* music_list);
void quit(MaComponents* ma_comp, Controller* controller);

} // namespace commands