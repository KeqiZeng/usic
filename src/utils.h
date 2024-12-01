#pragma once

#include <optional>
#include <string>
#include <string_view>

const int SECONDS_PER_MINUTE = 60;
namespace utils
{

void logMsg(std::string_view msg, std::string_view log_file, std::string_view source_file, int line, std::string_view func_name);
const std::string createTmpDefaultList();
const std::string createTmpBlankFile();
bool isLineInFile(std::string_view line, std::string_view file_name);
void deleteTmpFiles();
std::optional<int> timeStrToSec(std::string_view time_str);
std::optional<std::string> secToTimeStr(int seconds);
std::optional<std::string> removeExt(const std::string& music_name);

} // namespace utils
