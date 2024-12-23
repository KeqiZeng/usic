#pragma once
#include <filesystem>
#include <string>
#include <string_view>

#include "miniaudio.h"
#include "runtime_path.h"

inline const std::string ERROR_LOG_FILE = (std::filesystem::path(RUNTIME_PATH) / "error.log").string();
inline const std::string INFO_LOG_FILE  = (std::filesystem::path(RUNTIME_PATH) / "info.log").string();

enum class LogType
{
    ERROR,
    INFO,
};

void log(
    std::string_view message,
    LogType type,
    std::string_view file,
    int line,
    std::string_view func,
    ma_result result = MA_SUCCESS
);

#define LOG(message, type, ...)                                                                         \
    log(message, type, std::string_view{__FILE__}, __LINE__, std::string_view{__func__}, ##__VA_ARGS__)