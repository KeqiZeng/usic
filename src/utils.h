#pragma once

#include <optional>
#include <string>
#include <string_view>

const int SECONDS_PER_MINUTE = 60;
namespace utils
{

void logMsg(std::string_view msg, std::string_view file_name, std::string_view func_name);
const std::string createTmpDefaultList();
void deleteTmpDefaultList();
std::optional<int> timeStrToSec(std::string_view time_str);
std::optional<std::string> secToTimeStr(int seconds);
std::optional<std::string> extractMusicName(const std::string& music_path);

} // namespace utils
