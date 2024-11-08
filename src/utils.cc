#include "utils.h"
#include "fmt/chrono.h"
#include "fmt/core.h"
#include "fmt/os.h"
#include <chrono>
#include <optional>
#include <string>
#include <string_view>

namespace utils
{

void log(std::string_view msg, std::string_view file_name, std::string_view func_name)
{
    auto now        = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto local_time = *std::localtime(&now_time_t);
    auto message    = fmt::format("[{:%Y-%m-%d %H:%M:%S}] {}: {}\n", local_time, func_name, msg);

    auto log_file = fmt::output_file(file_name.data(), fmt::file::RDWR | fmt::file::CREATE | fmt::file::APPEND);
    log_file.print("{}", message);
    log_file.close();
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

} // namespace utils