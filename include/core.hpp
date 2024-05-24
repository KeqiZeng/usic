#pragma once

#include "miniaudio.h"
#include "music_list.hpp"

class SoundPack;
class MaComponents;
class EndFlag;

auto play_internal(ma_engine* pEngine, SoundPack* pSound_to_play,
                   const std::string& musicToPlay, std::string* musicPlaying,
                   MusicList* musicList, SoundPack* pSound_to_register,
                   EndFlag* endFlag, bool shuffle) -> ma_result;

auto cleanFunc(MaComponents* pMa, EndFlag* endFlag) -> void;
// auto cleanFunc(SoundPack* pSound_to_play, EndFlag* endFlag,
//                SoundPack* pSound_to_register) -> void;
