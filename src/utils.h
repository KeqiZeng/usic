#pragma once

#include <chrono>
#include <string>

#include "fmt/chrono.h"
#include "fmt/core.h"
#include "fmt/os.h"

#define SECONDS_PER_MINUTE 60
namespace utils {

void log(std::string_view msg, const std::string& filename);
int timeStr_to_sec(const std::string& timeStr);
std::string sec_to_timeStr(int seconds);

}  // namespace utils