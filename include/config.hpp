#pragma once

#include <string>

#include "constants.hpp"
#include "fmt/core.h"
#include "runtime.hpp"

class Config {
  bool ifRedundant;
  bool shuffle;
  std::string usicLibrary;
  std::string musicListPath;

 public:
  Config(bool _nonRedundant, bool _shuffle, const std::string& _usicLibrary,
         const std::string& _musicListPath)
      : ifRedundant(_nonRedundant), shuffle(_shuffle) {
    if (_usicLibrary.empty()) {
      error_log("usicLibrary can not be empty");
      std::exit(FATAL_ERROR);
    }
    if (_usicLibrary.back() != '/') {
      this->usicLibrary = fmt::format("{}{}", _usicLibrary, '/');
    } else {
      this->usicLibrary = _usicLibrary;
    }
    if (!_musicListPath.empty() && _musicListPath.back() != '/') {
      this->musicListPath =
          fmt::format("{}{}{}", this->usicLibrary, _musicListPath, '/');
    } else {
      this->musicListPath =
          fmt::format("{}{}", this->usicLibrary, _musicListPath);
    }
  }

  auto get_if_redundant() -> bool { return this->ifRedundant; }
  auto get_shuffle() -> bool { return this->shuffle; }
  auto get_usic_library() -> std::string { return this->usicLibrary; }
  auto get_music_list_path() -> std::string { return this->musicListPath; }

  // TODO
  //  auto load() -> void {}
};