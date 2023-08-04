#ifndef USIC_SERVER_H
#define USIC_SERVER_H

#include "miniaudio.h"

int cmdManager(ma_engine* pEngine, ma_sound* pSound, char** args, int numArgs, int fd_fromServer, bool* loopFlag);
ma_result play(ma_engine* pEngine, ma_sound* pSound, char** args, int numArgs);
int server(ma_engine* pEngine, ma_sound* pSound);

#endif // !USIC_SERVER_H

