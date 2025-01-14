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

/**
 * Logs a message to a file.
 *
 * @param message The message to log.
 * @param type The type of log message. If LogType::ERROR, the message will be
 * written to the error log file. Otherwise, it will be written to the info log
 * file.
 * @param file The file where the log function was called.
 * @param line The line number in the file where the log function was called.
 * @param func The name of the function where the log function was called.
 * @param result An optional ma_result value to log. If not MA_SUCCESS, the
 * value will be included in the log message.
 */
void log(
    std::string_view message,
    LogType type,
    std::string_view file,
    int line,
    std::string_view func,
    ma_result result = MA_SUCCESS
);

/**
 * A macro wrapper for the log function.
 *
 * @param message The message to log.
 * @param type The type of log message. If LogType::ERROR, the message will be
 * written to the error log file. Otherwise, it will be written to the info log
 * file.
 * @param ... Optional ma_result value to log. If not MA_SUCCESS, the value will
 * be included in the log message.
 */
#define LOG(message, type, ...)                                                                         \
    log(message, type, std::string_view{__FILE__}, __LINE__, std::string_view{__func__}, ##__VA_ARGS__)