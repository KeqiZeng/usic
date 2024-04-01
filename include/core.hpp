#pragma once

#include "miniaudio.h"
#include "music_list.hpp"

class SoundPack;
class MaComponents;
class SoundFinished;

auto play_internal(ma_engine* pEngine, SoundPack* pSound_to_play,
                   const std::string& musicToPlay, std::string* musicPlaying,
                   MusicList* musicList, SoundPack* pSound_to_register,
                   SoundFinished* soundFinished, bool shuffle) -> ma_result;

auto cleanFunc(MaComponents* pMa, SoundFinished* soundFinished) -> void;
// auto cleanFunc(SoundPack* pSound_to_play, SoundFinished* soundFinished,
//                SoundPack* pSound_to_register) -> void;
