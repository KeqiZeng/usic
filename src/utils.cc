#include "utils.h"
#include "config.h"
#include "fmt/chrono.h"
#include "fmt/core.h"
#include "fmt/os.h"
#include "runtime.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <optional>
#include <random>
#include <string>
#include <string_view>

namespace utils
{

void logMsg(std::string_view msg, std::string_view file_name, std::string_view func_name)
{
    auto now        = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto local_time = *std::localtime(&now_time_t);
    auto message    = fmt::format("[{:%Y-%m-%d %H:%M:%S}] {}: {}\n", local_time, func_name, msg);

    auto log_file = fmt::output_file(file_name.data(), fmt::file::RDWR | fmt::file::CREATE | fmt::file::APPEND);
    log_file.print("{}", message);
    log_file.close();
}

static const std::string generateRandomString(size_t length)
{
    static const std::string CHARS = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, CHARS.size() - 1);

    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result += CHARS[dis(gen)];
    }
    return result;
}

const std::string createTmpDefaultList()
{
    std::string random_name = "usic_" + generateRandomString(10) + ".txt";
    std::string temp_file   = std::filesystem::temp_directory_path() / random_name;

    std::ofstream outfile(temp_file);
    if (!outfile.is_open()) {
        log(fmt::format("Failed to open temporary file: {}", temp_file), LogType::ERROR, __func__);
        throw std::runtime_error(fmt::format("Failed to create temporary file: {}", temp_file));
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(USIC_LIBRARY)) {
        if (!entry.is_regular_file()) continue;

        std::string ext = entry.path().extension().string();
        std::ranges::transform(ext, ext.begin(), ::tolower);

        if (ext == ".mp3" || ext == ".flac" || ext == ".wav") {
            outfile << entry.path().filename().string() << '\n';
            if (outfile.fail()) {
                log(fmt::format("Failed to write to temporary file: {}", temp_file), LogType::ERROR, __func__);
                throw std::runtime_error(fmt::format("Failed to write to temporary file: {}", temp_file));
            }
        }
    }

    outfile.close();
    if (outfile.fail()) {
        log(fmt::format("Failed to close temporary file: {}", temp_file), LogType::ERROR, __func__);
        throw std::runtime_error(fmt::format("Failed to close temporary file: {}", temp_file));
    }

    return temp_file;
}

void deleteTmpDefaultList()
{
    const auto TMP_DIR = std::filesystem::temp_directory_path();
    for (const auto& entry : std::filesystem::directory_iterator(TMP_DIR)) {
        if (!entry.is_regular_file()) continue;

        const std::string FILENAME = entry.path().filename().string();
        if (FILENAME.starts_with("usic_") && FILENAME.ends_with(".txt")) {
            std::error_code ec;
            std::filesystem::remove(entry.path(), ec);
            if (ec) {
                log(fmt::format("Failed to delete temporary file {}: {}", entry.path().string(), ec.message()),
                    LogType::ERROR,
                    __func__);
            }
        }
    }
}

std::optional<int> timeStrToSec(std::string_view time_str)
{
    int min = 0;
    int sec = 0;

    std::istringstream iss(time_str.data());
    char colon;
    if (!(iss >> min >> colon >> sec) || colon != ':') {
        return std::nullopt; // failed conversion
    }

    // minutes should not be less than 0
    if (min < 0) { return std::nullopt; }

    // seconds should be 0-60
    if (sec < 0 || sec > SECONDS_PER_MINUTE) { return -1; }

    return min * SECONDS_PER_MINUTE + sec;
}

std::optional<std::string> secToTimeStr(int seconds)
{
    if (seconds < 0) { return std::nullopt; }

    int min = seconds / SECONDS_PER_MINUTE;
    int sec = seconds % SECONDS_PER_MINUTE;

    return fmt::format("{:d}:{:02d}", min, sec);
}

std::optional<std::string> extractMusicName(const std::string& music_path)
{
    std::filesystem::path path(music_path);
    if (path.empty() || !path.has_filename() || !path.has_extension()) { return std::nullopt; }
    return path.stem().string();
}

} // namespace utils