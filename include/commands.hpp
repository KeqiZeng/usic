#pragma once

#include "miniaudio.h"
#include "music_list.hpp"

struct SoundInitNotification;
class SoundPack;
class MaComponents;
struct Progress;

auto play(ma_engine* pEngine, SoundPack* pSound_to_play,
          const std::string& musicToPlay, std::string* musicPlaying,
          MusicList* musicList, SoundPack* pSound_to_register,
          bool shuffle) -> ma_result;
auto play_prev(MaComponents* pMa, std::string* musicPlaying,
               MusicList* musicList, bool shuffle = false) -> ma_result;

auto play_later(const std::string& music, MusicList* music_list) -> void;
auto play_next(MaComponents* pMa) -> ma_result;
auto pause_resume(MaComponents* pMa) -> ma_result;
auto move_cursor(MaComponents* pMa, int seconds) -> ma_result;
auto set_cursor(MaComponents* pMa, const std::string& time) -> ma_result;
auto get_current_progress(MaComponents* pMa,
                          Progress* currentProgress) -> ma_result;
auto adjust_volume(ma_engine* pEngine, float diff) -> ma_result;
auto get_volume(ma_engine* pEngine, float* volume) -> ma_result;
auto mute_toggle(ma_engine* pEngine) -> ma_result;