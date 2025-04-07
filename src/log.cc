#include "log.h"

#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <mutex>
#include <string_view>

static std::mutex g_log_mutex;

static void logMsg(
    std::string_view msg,
    std::string_view log_file,
    std::string_view source_file,
    int line,
    std::string_view func_name,
    ma_result result
)
{
    // Filter out control characters but keep all other characters including Unicode
    std::string filtered_msg;
    filtered_msg.reserve(msg.size());
    for (unsigned char c : msg) {
        if (!std::iscntrl(c) || c == '\n' || c == '\t' || c == '\r') {
            filtered_msg += c;
        }
    }

    if (result != MA_SUCCESS) {
        filtered_msg = std::format("{} ({})", filtered_msg, ma_result_description(result));
    }

    const auto NOW        = std::chrono::system_clock::now();
    const auto NOW_TIME_T = std::chrono::system_clock::to_time_t(NOW);
    const auto LOCAL_TIME = *std::localtime(&NOW_TIME_T);

    const auto MESSAGE = std::format(
        "[{0:04d}-{1:02d}-{2:02d} {3:02d}:{4:02d}:{5:02d} {6}:{7}:{8}] {9}\n",
        LOCAL_TIME.tm_year + 1900,                              // 0
        LOCAL_TIME.tm_mon + 1,                                  // 1
        LOCAL_TIME.tm_mday,                                     // 2
        LOCAL_TIME.tm_hour,                                     // 3
        LOCAL_TIME.tm_min,                                      // 4
        LOCAL_TIME.tm_sec,                                      // 5
        std::filesystem::path{source_file}.filename().string(), // 6
        line,                                                   // 7
        func_name,                                              // 8
        filtered_msg                                            // 9
    );

    std::ofstream out_file(log_file.data(), std::ios::app);
    if (!out_file) {
        throw std::runtime_error("failed to open log file");
    }

    out_file << MESSAGE;
    out_file.close();
}

void log(
    std::string_view message,
    LogType type,
    std::string_view file,
    int line,
    std::string_view func,
    ma_result result
)
{
    std::lock_guard<std::mutex> lock(g_log_mutex);
    switch (type) {
    case LogType::ERROR:
        logMsg(message, ERROR_LOG_FILE, file, line, func, result);
        break;
    case LogType::INFO:
        logMsg(message, INFO_LOG_FILE, file, line, func, result);
        break;
    default:
        break;
    }
}

void removeLogFiles()
{
    if (std::filesystem::exists(ERROR_LOG_FILE)) {
        std::filesystem::remove(ERROR_LOG_FILE);
    }
    if (std::filesystem::exists(INFO_LOG_FILE)) {
        std::filesystem::remove(INFO_LOG_FILE);
    }
}
