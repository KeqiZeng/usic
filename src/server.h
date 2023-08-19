#ifndef USIC_SERVER_H
#define USIC_SERVER_H

#include "miniaudio.h"

#include "./musicList.h"

ma_result cmdManager(ma_engine* pEngine, ma_sound* pSound, char** args, int numArgs, int fd_fromServer, bool* loopFlag, MusicNode* currentMusicNode, MusicList* defaultMusicList, MusicList* laterPlayed);
ma_result play(ma_engine* pEngine, ma_sound* pSound, char* music, MusicNode* currentMusic);
MusicNode* loadNextMusic(MusicList* defaultMusicList, MusicList* laterPlayed);
ma_result playList(ma_engine* pEngine, ma_sound* pSound, char** args, int numArgs, MusicNode* currentMusicNode, MusicList* defaultMusicList, MusicList* laterPlayed);
ma_result server(ma_engine* pEngine, ma_sound* pSound);

#endif // !USIC_SERVER_H

