#pragma once
#include "runtime.h"
#include <string_view>

namespace tools
{

void addMusicToList(std::string_view list_name, Config* config);
void fuzzyPlay(std::string_view usic_command, Config* config);

} // namespace tools
