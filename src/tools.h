#pragma once
#include <string_view>

bool handleFlagOrTool(int argc, char* argv[]);

namespace Tools
{

void addMusicToList(std::string_view list_name);
void fuzzyPlay(std::string_view usic_command);
void fuzzyPlayLater(std::string_view usic_command);

} // namespace Tools
