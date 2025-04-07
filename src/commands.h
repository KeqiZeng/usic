#pragma once
#include "core.h"
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Commands
{

ma_result play(std::string_view audio);
void pause();
ma_result playNext();
ma_result playPrev();
ma_result playLater(std::string_view audio);

void load(std::string_view list);
std::vector<std::string> getList();

ma_result seekForward();
ma_result seekBackward();
ma_result seekTo(std::string_view time);
std::optional<const std::string> getProgressBar();

ma_result volumeUp();
ma_result volumeDown();
ma_result setVolume(std::string_view volume);
float getVolume();
ma_result mute();

void setMode(PlayMode mode);
PlayMode getMode();

void quit();

} // namespace Commands
