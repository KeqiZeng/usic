#pragma once

#include "miniaudio.h"
#include <string>
#include <string_view>

namespace Utils
{

ma_result reinitDecoder(std::string_view filename, ma_decoder_config* config, ma_decoder* decoder);
ma_result seekToStart(ma_decoder* decoder) noexcept;
ma_result seekToEnd(ma_decoder* decoder);
std::string getWAVFileName(std::string_view filename);
void createFile(std::string_view filename);

} // namespace Utils