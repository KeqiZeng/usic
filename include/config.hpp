#pragma once

#include <string>

#include "constants.hpp"
#include "fmt/core.h"
#include "runtime.hpp"

class Config {
  bool ifRedundant;
  bool shuffle;
  std::string usicLibrary;
  std::string playListPath;

 public:
  Config(bool _nonRedundant, bool _shuffle, const std::string& _usicLibrary)
      : ifRedundant(_nonRedundant), shuffle(_shuffle) {
    if (_usicLibrary.empty()) {
      log("usicLibrary can not be empty", LogType::ERROR);
      std::exit(FATAL_ERROR);
    }
    if (_usicLibrary.back() != '/') {
      this->usicLibrary = fmt::format("{}{}", _usicLibrary, '/');
    } else {
      this->usicLibrary = _usicLibrary;
    }
    this->playListPath =
        fmt::format("{}{}", this->usicLibrary, PLAY_LISTS_PATH);
  }

  auto get_if_redundant() -> bool { return this->ifRedundant; }
  auto get_shuffle() -> bool { return this->shuffle; }
  auto get_usic_library() -> std::string { return this->usicLibrary; }
  auto get_playList_path() -> std::string { return this->playListPath; }

  // TODO
  //  auto load() -> void {}
};