#pragma once
#include <string>

#include "config.h"
#include "core.h"
#include "miniaudio.h"
#include "music_list.h"
#include "utils.h"

class Progress {
  std::string str;
  float percent;

 public:
  void init(const std::string& _str, float _percent);
  const std::string make_bar();
};

ma_result play(MaComponents* pMa, const std::string& musicToPlay,
               std::string* musicPlaying, MusicList* musicList,
               QuitControl* quitC, Config* config);

void play_later(Config* config, const std::string& music,
                MusicList* music_list);
ma_result play_next(MaComponents* pMa);
ma_result play_prev(MaComponents* pMa, MusicList* musicList);
ma_result pause_resume(MaComponents* pMa);
ma_result stop(MaComponents* pMa);
ma_result move_cursor(MaComponents* pMa, int seconds);
ma_result set_cursor(MaComponents* pMa, const std::string& time);
ma_result get_current_progress(MaComponents* pMa, Progress* currentProgress);
ma_result adjust_volume(ma_engine* pEngine, float diff);
ma_result get_volume(ma_engine* pEngine, float* volume);
ma_result mute_toggle(ma_engine* pEngine);
void quit(MaComponents* pMa, QuitControl* quitC);