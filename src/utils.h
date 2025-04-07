#pragma once

#include "miniaudio.h"
#include <string>
#include <string_view>

namespace Utils
{

void createFile(std::string_view filename);
const std::string createTmpBlankFile();
const std::string createTmpDefaultList();
void removeTmpFiles();
bool isLineInFile(std::string_view line, std::string_view file_name);
bool isServerRunning();
std::optional<unsigned int> timeStrToSec(std::string_view time_str);
std::optional<std::string> secToTimeStr(unsigned int seconds);
bool commandEq(const std::string& command, const std::string_view& target);
const std::string concatenateArgs(int argc, char* argv[]);
ma_result reinitDecoder(std::string_view filename, ma_decoder_config* config, ma_decoder* decoder);
ma_result seekToStart(ma_decoder* decoder) noexcept;
ma_result seekToEnd(ma_decoder* decoder);
std::string getWAVFileName(std::string_view filename);

} // namespace Utils
