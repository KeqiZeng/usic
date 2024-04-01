#include "runtime.hpp"

#include <filesystem>

#include "constants.hpp"
#include "fmt/chrono.h"
#include "fmt/core.h"
#include "fmt/os.h"

auto setup() -> void {
  if (!std::getenv("HOME")) {
    error_log("Failed to get environment variable: HOME");
    std::exit(FATAL_ERROR);
  }
  if (!std::filesystem::exists(LOGPATH)) {
    std::filesystem::create_directory(LOGPATH);
  }
  if (std::filesystem::exists(LOGFILE)) {
    std::filesystem::remove(LOGFILE);
  }
}

auto error_log(std::string_view error) -> void {
  auto errorLog = fmt::output_file(
      LOGFILE, fmt::file::RDWR | fmt::file::CREATE | fmt::file::APPEND);

  auto now = std::chrono::system_clock::now();
  auto now_time_t = std::chrono::system_clock::to_time_t(now);
  auto localTime = *std::localtime(&now_time_t);
  auto errMessage = fmt::format("[{:%Y-%m-%d %H:%M:%S}] {}", localTime, error);
  errorLog.print("{}\n", errMessage);
  errorLog.close();
}