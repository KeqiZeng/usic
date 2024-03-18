#pragma once
#include <string>

#include "fmt/core.h"

#define LOADING_FLAGS \
  MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC

const int FATAL_ERROR = -1;
const std::string logPath =
    fmt::format("{}/{}", std::getenv("HOME"), ".local/state/usic");
const std::string logFile =
    fmt::format("{}/{}", std::getenv("HOME"), ".local/state/usic/error.log");
const int SECONDS_PER_MINUTE = 60;