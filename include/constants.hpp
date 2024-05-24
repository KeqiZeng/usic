#pragma once
#include <string>

#include "fmt/core.h"

#define LOADING_FLAGS \
  MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC

const int FATAL_ERROR = -1;
const std::string RUNTIME_PATH =
    fmt::format("{}/{}", std::getenv("HOME"), ".local/state/usic");
const std::string ERROR_LOG_FILE =
    fmt::format("{}/{}", RUNTIME_PATH, "error.log");
const std::string INFO_LOG_FILE =
    fmt::format("{}/{}", RUNTIME_PATH, "info.log");
const std::string PIPE_TO_SERVER =
    fmt::format("{}/{}", RUNTIME_PATH, "pipe_to_server");
const std::string PIPE_TO_CLIENT =
    fmt::format("{}/{}", RUNTIME_PATH, "pipe_from_server");

const std::string PLAY_LISTS_PATH = "playLists/";
const int SECONDS_PER_MINUTE = 60;