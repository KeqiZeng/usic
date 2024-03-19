#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "building_flags.hpp"
#include "commands.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "fmt/core.h"
#include "miniaudio.h"
#include "music_list.hpp"
#include "runtime.hpp"

auto main() -> int {
  setup();
  std::unique_ptr<MaComponents> pMa(new MaComponents());
  ma_result result = pMa->ma_comp_init_engine();

  std::unique_ptr<MusicList> musicList(
      new MusicList("/Users/ketch/Music/usic/playList/Default.txt"));
  if (musicList->is_empty()) {
    error_log("Failed to load music list");
    std::exit(FATAL_ERROR);
  }

  std::unique_ptr<std::string> musicPlaying(new std::string);

  const std::string& musicToPlay = musicList->random_out()->get_music();
  result = play(pMa->pEngine.get(), pMa->pSound_to_play.get(), musicToPlay,
                musicPlaying.get(), musicList.get(),
                pMa->pSound_to_register.get(), true);
  if (result != MA_SUCCESS) {
    error_log(fmt::format("Failed to play music: {}", musicToPlay));
    std::exit(FATAL_ERROR);
  }

  fmt::print("Press Enter to continue...");
  std::cin.get();

  play_next(pMa.get());
  fmt::print("Press Enter to continue...");
  std::cin.get();
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

  return 0;
}
