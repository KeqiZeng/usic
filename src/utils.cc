#include "utils.h"

namespace utils {

void log(std::string_view msg, std::string_view filename,
         std::string_view funcName) {
  auto now = std::chrono::system_clock::now();
  auto now_time_t = std::chrono::system_clock::to_time_t(now);
  auto localTime = *std::localtime(&now_time_t);
  auto logMessage =
      fmt::format("[{:%Y-%m-%d %H:%M:%S}] {}: {}\n", localTime, funcName, msg);

  auto logFile = fmt::output_file(
      filename.data(), fmt::file::RDWR | fmt::file::CREATE | fmt::file::APPEND);
  logFile.print("{}", logMessage);
  logFile.close();
}

int timeStr_to_sec(const std::string& timeStr) {
  int min = 0;
  int sec = 0;

  std::istringstream iss(timeStr);
  char colon;
  if (!(iss >> min >> colon >> sec) || colon != ':') {
    return -1;  // failed conversion
  }

  // minutes should not be less than 0
  if (min < 0) {
    return -1;
  }

  // seconds should be 0-60
  if (sec < 0 || sec > SECONDS_PER_MINUTE) {
    return -1;
  }

  return min * SECONDS_PER_MINUTE + sec;
}

std::string sec_to_timeStr(int seconds) {
  if (seconds < 0) {
    return "";
  }

  int min = seconds / SECONDS_PER_MINUTE;
  int sec = seconds % SECONDS_PER_MINUTE;

  return fmt::format("{:d}:{:02d}", min, sec);
}

}  // namespace utils