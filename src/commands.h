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

namespace Commands {

void load(const std::string& listPath, MusicList* musicList, Config* config);
ma_result play(MaComponents* pMa, const std::string& musicToPlay,
               std::string* musicPlaying, MusicList* musicList,
               QuitControl* quitC, Config* config);

void play_later(Config* config, const std::string& music,
                MusicList* music_list);
ma_result play_next(MaComponents* pMa);
ma_result play_prev(MaComponents* pMa, MusicList* musicList);
ma_result pause_resume(MaComponents* pMa);
ma_result cursor_forward(MaComponents* pMa);
ma_result cursor_backward(MaComponents* pMa);
ma_result set_cursor(MaComponents* pMa, const std::string& time);
ma_result get_current_progress(MaComponents* pMa, Progress* currentProgress);
ma_result volume_up(ma_engine* pEngine);
ma_result volume_down(ma_engine* pEngine);
ma_result set_volume(ma_engine* pEngine, const std::string& volumeStr);
ma_result get_volume(ma_engine* pEngine, float* volume);
ma_result mute(ma_engine* pEngine);
void set_random(Config* config);
void set_repetitive(Config* config);
std::vector<std::string> get_list(MusicList* musicList);
void quit(MaComponents* pMa, QuitControl* quitC);

}  // namespace Commands