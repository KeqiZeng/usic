#ifndef USIC_UTILS_H
#define USIC_UTILS_H

#include "miniaudio.h"

int serverIsRunning();
void redirectStderr();
void setupRuntime();
void setPipePath(const char* pipeName, char* pipePath);
char* addQuotesIfNeeded(const char* str);
char** argsParser(const char* str, int* numArgs);
ma_result playToggle(ma_sound* pSound);
ma_result getCurrentProgress(ma_sound* pSound, char** progress);
ma_result moveCursor(ma_engine* pEngine, ma_sound* pSound, int seconds);
ma_result setCursor(ma_engine* pEngine, ma_sound* pSound, const char* time);
ma_result adjustVolume(ma_engine* pEngine, float diff);
ma_result getVolume(ma_engine* pEngine, char** volume);

#endif // !USIC_UTILS_H
