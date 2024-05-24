#pragma once

#include "components.hpp"
#include "miniaudio.h"
#include "music_list.hpp"

auto play(MaComponents* pMa, const std::string& musicToPlay,
          std::string* musicPlaying, MusicList* musicList, EndFlag* endFlag,
          Config* config) -> ma_result;
auto play_later(Config* config, const std::string& music,
                MusicList* music_list) -> void;
auto play_next(MaComponents* pMa) -> ma_result;
auto play_prev(MaComponents* pMa, MusicList* musicList) -> ma_result;
auto pause_resume(MaComponents* pMa) -> ma_result;
auto move_cursor(MaComponents* pMa, int seconds) -> ma_result;
auto set_cursor(MaComponents* pMa, const std::string& time) -> ma_result;
auto get_current_progress(MaComponents* pMa,
                          Progress* currentProgress) -> ma_result;
auto adjust_volume(ma_engine* pEngine, float diff) -> ma_result;
auto get_volume(ma_engine* pEngine, float* volume) -> ma_result;
auto mute_toggle(ma_engine* pEngine) -> ma_result;
auto quit(EndFlag* endFlag) -> void;