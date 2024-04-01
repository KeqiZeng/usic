#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "building_flags.hpp"
#include "commands.hpp"
#include "components.hpp"
#include "config.hpp"
#include "constants.hpp"
#include "core.hpp"
#include "fmt/core.h"
#include "miniaudio.h"
#include "music_list.hpp"
#include "runtime.hpp"

auto main() -> int {
  setup();
  std::unique_ptr<Config> config(
      new Config(false, true, "/Users/ketch/Music/usic", "playLists"));
  std::unique_ptr<MaComponents> pMa(new MaComponents());
  ma_result result = pMa->ma_comp_init_engine();

  std::unique_ptr<MusicList> musicList(new MusicList(
      config.get(),
      fmt::format("{}{}", config->get_music_list_path(), "Default_.txt")));
  if (musicList->is_empty()) {
    error_log("Failed to load music list");
    std::exit(FATAL_ERROR);
  }

  std::unique_ptr<std::string> musicPlaying(new std::string);

  std::unique_ptr<SoundFinished> soundFinished(new SoundFinished);

  const std::string& musicToPlay = musicList->random_out()->get_music();
  result = play(pMa.get(), musicToPlay, musicPlaying.get(), musicList.get(),
                soundFinished.get(), config.get());
  if (result != MA_SUCCESS) {
    error_log(fmt::format("Failed to play music: {}", musicToPlay));
    std::exit(FATAL_ERROR);
  }

  std::thread cleaner(cleanFunc, pMa.get(), soundFinished.get());
  cleaner.detach();

  fmt::print("Press Enter to continue...");
  std::cin.get();
  // pause_resume(pMa.get());
  // fmt::print("Press Enter to continue...");
  // std::cin.get();
  play_next(pMa.get());
  fmt::print("Press Enter to continue...");
  std::cin.get();
  // std::unique_ptr<Progress> current_progress(new Progress);
  // get_current_progress(pMa.get(), current_progress.get());
  // fmt::print("{}, {:.2f}\n", current_progress->str,
  // current_progress->percent); fmt::print("Press Enter to continue...");
  // std::cin.get();
  // get_current_progress(pMa.get(), current_progress.get());
  // fmt::print("{}, {:.2f}\n", current_progress->str,
  // current_progress->percent); fmt::print("Press Enter to continue...");
  // std::cin.get();
  // set_cursor(pMa.get(), "3:50");
  // fmt::print("Press Enter to continue...");
  // std::cin.get();
  // move_cursor(pMa.get(), 10);
  // fmt::print("Press Enter to continue...");
  // std::cin.get();
  // move_cursor(pMa.get(), -10);
  // fmt::print("Press Enter to continue...");
  // std::cin.get();
  // pause_resume(pMa.get());
  // fmt::print("Press Enter to continue...");
  // std::cin.get();
  // pause_resume(pMa.get());
  // fmt::print("Press Enter to continue...");
  // std::cin.get();
  play_prev(pMa.get(), musicList.get());
  fmt::print("Press Enter to continue...");
  std::cin.get();
  // result = play(pMa.get(), "/Users/ketch/Music/usic/Beyond - 情人.flac",
  //               musicPlaying.get(), musicList.get(), soundFinished.get(),
  //               config.get());
  // fmt::print("Press Enter to continue...");
  // std::cin.get();
  const std::string& musicToPlay_2 = musicList->random_out()->get_music();
  result = play(pMa.get(), musicToPlay_2, musicPlaying.get(), musicList.get(),
                soundFinished.get(), config.get());
  // float volume = 0.0F;
  // get_volume(pMa->pEngine.get(), &volume);
  // fmt::print("volume: {:.2f}\n", volume);
  // fmt::print("Press Enter to continue...");
  // std::cin.get();
  // adjust_volume(pMa->pEngine.get(), -0.5);
  // get_volume(pMa->pEngine.get(), &volume);
  // fmt::print("volume: {:.2f}\n", volume);
  // fmt::print("Press Enter to continue...");
  // std::cin.get();
  // adjust_volume(pMa->pEngine.get(), 0.5);
  // get_volume(pMa->pEngine.get(), &volume);
  // fmt::print("volume: {:.2f}\n", volume);
  // fmt::print("Press Enter to continue...");
  // std::cin.get();
  // mute_toggle(pMa->pEngine.get());
  // fmt::print("Press Enter to continue...");
  // std::cin.get();
  // mute_toggle(pMa->pEngine.get());
  // fmt::print("Press Enter to continue...");
  // std::cin.get();

  fmt::print("Press Enter to continue...");
  std::cin.get();
  quit(soundFinished.get());
  fmt::print("Press Enter to continue...");
  std::cin.get();
  return 0;
}
