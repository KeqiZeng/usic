#include <iostream>
#include <string>
#include <thread>

#include "commands.h"
#include "config.h"
#include "core.h"
#include "fmt/core.h"
#include "music_list.h"
#include "runtime.h"

/* marcos */
#define MA_ENABLE_ONLY_SPECIFIC_BACKENDS
#define MA_ENABLE_COREAUDIO
#define MA_ENABLE_ALSA
#define MA_NO_ENCODING
#define MA_NO_GENERATION
#define MINIAUDIO_IMPLEMENTATION

// miniaudio should be included after MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

int main() {
  setup_runtime();
  std::unique_ptr<Config> config(
      new Config(false, true, USIC_LIBARY, PLAY_LISTS_PATH));
  std::unique_ptr<MaComponents> pMa(new MaComponents());
  ma_result result = pMa->ma_comp_init_engine();

  std::unique_ptr<MusicList> musicList(new MusicList(
      config.get(),
      fmt::format("{}{}", config->get_playList_path(), "Default.txt")));
  if (musicList->is_empty()) {
    log("Failed to load music list", LogType::ERROR);
    std::exit(FATAL_ERROR);
  }

  std::unique_ptr<std::string> musicPlaying(new std::string);

  std::unique_ptr<QuitControl> quitC(new QuitControl);

  const std::string& musicToPlay = musicList->random_out()->get_music();
  result = play(pMa.get(), musicToPlay, musicPlaying.get(), musicList.get(),
                quitC.get(), config.get());
  if (result != MA_SUCCESS) {
    log(fmt::format("Failed to play music: {}", musicToPlay), LogType::ERROR);
    std::exit(FATAL_ERROR);
  }

  std::thread cleaner(cleanFunc, pMa.get(), quitC.get());
  cleaner.detach();

  fmt::print("Press Enter to continue...");
  std::cin.get();
  pause_resume(pMa.get());
  fmt::print("Press Enter to continue...");
  std::cin.get();
  play_next(pMa.get());
  fmt::print("Press Enter to continue...");
  std::cin.get();
  std::unique_ptr<Progress> current_progress(new Progress);
  get_current_progress(pMa.get(), current_progress.get());
  fmt::print("{}", current_progress->make_bar());
  fmt::print("Press Enter to continue...");
  std::cin.get();
  get_current_progress(pMa.get(), current_progress.get());
  fmt::print("{}", current_progress->make_bar());
  fmt::print("Press Enter to continue...");
  std::cin.get();
  set_cursor(pMa.get(), "3:50");
  fmt::print("Press Enter to continue...");
  std::cin.get();
  move_cursor(pMa.get(), 10);
  fmt::print("Press Enter to continue...");
  std::cin.get();
  move_cursor(pMa.get(), -10);
  fmt::print("Press Enter to continue...");
  std::cin.get();
  pause_resume(pMa.get());
  fmt::print("Press Enter to continue...");
  std::cin.get();
  pause_resume(pMa.get());
  fmt::print("Press Enter to continue...");
  std::cin.get();
  play_prev(pMa.get(), musicList.get());
  fmt::print("Press Enter to continue...");
  std::cin.get();
  const std::string& musicToPlay_2 = musicList->random_out()->get_music();
  result = play(pMa.get(), musicToPlay_2, musicPlaying.get(), musicList.get(),
                quitC.get(), config.get());
  fmt::print("Press Enter to continue...");
  std::cin.get();
  float volume = 0.0F;
  get_volume(pMa->pEngine.get(), &volume);
  fmt::print("volume: {:.2f}\n", volume);
  fmt::print("Press Enter to continue...");
  std::cin.get();
  adjust_volume(pMa->pEngine.get(), -0.5);
  get_volume(pMa->pEngine.get(), &volume);
  fmt::print("volume: {:.2f}\n", volume);
  fmt::print("Press Enter to continue...");
  std::cin.get();
  adjust_volume(pMa->pEngine.get(), 0.5);
  get_volume(pMa->pEngine.get(), &volume);
  fmt::print("volume: {:.2f}\n", volume);
  fmt::print("Press Enter to continue...");
  std::cin.get();
  mute_toggle(pMa->pEngine.get());
  fmt::print("Press Enter to continue...");
  std::cin.get();
  mute_toggle(pMa->pEngine.get());
  fmt::print("Press Enter to continue...");
  std::cin.get();
  quit(pMa.get(), quitC.get());
  fmt::print("Press Enter to continue...");
  std::cin.get();
  return 0;
}