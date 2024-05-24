#include "runtime.hpp"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <future>

#include "config.hpp"
#include "constants.hpp"
#include "fmt/chrono.h"
#include "fmt/core.h"
#include "fmt/os.h"

auto server_is_running() -> bool {
  // Execute ps command and read its output
  FILE* fp = popen("/bin/ps -e -o comm=", "r");
  if (fp == nullptr) {
    log("Failed to execute ps command", LogType::ERROR);
    std::exit(FATAL_ERROR);
  }

  // Read the output of the command
  char buffer[256];
  while (std::fgets(buffer, sizeof(buffer), fp) != nullptr) {
    // Remove the trailing newline character
    buffer[std::strcspn(buffer, "\n")] = '\0';

    if (std::strcmp(buffer, "usic server") == 0) {
      return true;  // server is running
    }
  }

  pclose(fp);

  return false;  // server is not running
}

auto setup_runtime() -> void {
  if (!std::getenv("HOME")) {
    fmt::print(stderr, "Failed to get environment variable: HOME\n");
    std::exit(FATAL_ERROR);
  }
  // create runtime path
  if (!std::filesystem::exists(RUNTIME_PATH)) {
    std::filesystem::create_directory(RUNTIME_PATH);
  }

  // remove log files for initialization
  if (std::filesystem::exists(ERROR_LOG_FILE)) {
    std::filesystem::remove(ERROR_LOG_FILE);
  }
  if (std::filesystem::exists(INFO_LOG_FILE)) {
    std::filesystem::remove(INFO_LOG_FILE);
  }
}

//   if (pipeToServer->setup() == -1) {
//     log("Failed to setup pipeToServer", LogType::ERROR);
//     std::exit(FATAL_ERROR);
//   }
//   if (pipeToClient->setup() == -1) {
//     log("Failed to setup pipeToClient", LogType::ERROR);
//     std::exit(FATAL_ERROR);
//   }
// }

auto static error_log(std::string_view error) -> void {
  auto errorLog = fmt::output_file(
      ERROR_LOG_FILE, fmt::file::RDWR | fmt::file::CREATE | fmt::file::APPEND);

  auto now = std::chrono::system_clock::now();
  auto now_time_t = std::chrono::system_clock::to_time_t(now);
  auto localTime = *std::localtime(&now_time_t);
  auto errMessage = fmt::format("[{:%Y-%m-%d %H:%M:%S}] {}", localTime, error);
  errorLog.print("{}\n", errMessage);
  errorLog.close();
}

auto static info_log(std::string_view info) -> void {
  auto infoLog = fmt::output_file(
      INFO_LOG_FILE, fmt::file::RDWR | fmt::file::CREATE | fmt::file::APPEND);

  auto now = std::chrono::system_clock::now();
  auto now_time_t = std::chrono::system_clock::to_time_t(now);
  auto localTime = *std::localtime(&now_time_t);
  auto infoMessage = fmt::format("[{:%Y-%m-%d %H:%M:%S}] {}", localTime, info);
  infoLog.print("{}\n", infoMessage);
  infoLog.close();
}

auto log(std::string_view message, LogType type) -> void {
  switch (type) {
    case LogType::ERROR:
      std::future<void>{} = std::async(std::launch::async, error_log, message);
      break;
    case LogType::INFO:
      std::future<void>{} = std::async(std::launch::async, info_log, message);
      break;
  }
}
