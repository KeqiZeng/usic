#include "server.h"

#include <thread>

std::string get_command(NamedPipe* pipeToServer, NamedPipe* pipeToClient) {
  pipeToServer->open_pipe(OpenMode::RD_ONLY_BLOCK);
  pipeToClient->open_pipe(OpenMode::WR_ONLY_BLOCK);
  std::string cmd = pipeToServer->readOut();
  pipeToServer->close_pipe();
  pipeToClient->close_pipe();
  return cmd;
}

void sendMsgToClient(const std::string& msg, NamedPipe* pipeToServer,
                     NamedPipe* pipeToClient) {
  pipeToServer->open_pipe(OpenMode::RD_ONLY_BLOCK);
  pipeToClient->open_pipe(OpenMode::WR_ONLY_BLOCK);
  pipeToClient->writeIn(msg);
  if (pipeToServer->readOut() == GOT) {
    pipeToClient->writeIn(OVER);
  }
  pipeToServer->close_pipe();
  pipeToClient->close_pipe();
}

void sendMsgToClient(const std::vector<std::string>* msg,
                     NamedPipe* pipeToServer, NamedPipe* pipeToClient) {
  if (msg->size() == 0) {
    log("got an empty message", LogType::ERROR, __func__);
    return;
  }
  pipeToServer->open_pipe(OpenMode::RD_ONLY_BLOCK);
  pipeToClient->open_pipe(OpenMode::WR_ONLY_BLOCK);
  pipeToClient->writeIn(msg->at(0));
  std::string response = pipeToServer->readOut();
  if (response == GOT) {
    for (size_t i = 1; i < msg->size(); i++) {
      pipeToClient->writeIn(msg->at(i));
      response = pipeToServer->readOut();
    }
  }
  pipeToClient->writeIn(OVER);
  pipeToServer->close_pipe();
  pipeToClient->close_pipe();
}

/* Split command into individual arguments, by space but skip the space inside
 * quotes */
static std::vector<std::string> parse_command(std::string& cmd) {
  std::vector<std::string> args;
  std::string current;
  bool inQuotes = false;

  for (char c : cmd) {
    if (c == '\"') {
      // skip space inside quotes
      inQuotes = !inQuotes;
    } else if (c == ' ' && !inQuotes) {
      if (!current.empty()) {
        args.push_back(current);
        current.clear();
      }
    } else {
      current += c;
    }
  }

  if (!current.empty()) {
    args.push_back(current);
  }

  return args;
}

static void logErrorAndSendToClient(const std::string& err,
                                    NamedPipe* pipeToServer,
                                    NamedPipe* pipeToClient) {
  log(err, LogType::ERROR, __func__);
  sendMsgToClient(err, pipeToServer, pipeToClient);
}

void handle_command(std::string& cmd, NamedPipe* pipeToServer,
                    NamedPipe* pipeToClient, MaComponents* pMa,
                    std::string* musicPlaying, MusicList* musicList,
                    QuitControl* quitC, Config* config) {
  std::vector<std::string> args = parse_command(cmd);
  const std::string& subCmd = args.at(0);
  if (subCmd == LOAD ||
      std::ranges::find(COMMANDS.at(LOAD), subCmd) != COMMANDS.at(LOAD).end()) {
    if (args.size() >= 2) {
      Commands::load(args.at(1), musicList, config);
      sendMsgToClient(NO_MESSAGE, pipeToServer, pipeToClient);
    } else {
      logErrorAndSendToClient(fmt::format("{}: not enough arguments.", LOAD),
                              pipeToServer, pipeToClient);
    }
  } else if (subCmd == PLAY || std::ranges::find(COMMANDS.at(PLAY), subCmd) !=
                                   COMMANDS.at(PLAY).end()) {
    if (args.size() >= 2) {
      const std::string& musicToPlay = args.at(1);
      ma_result result = Commands::play(pMa, musicToPlay, musicPlaying,
                                        musicList, quitC, config);
      if (result != MA_SUCCESS) {
        logErrorAndSendToClient(
            fmt::format("Failed to play music: {}", musicToPlay), pipeToServer,
            pipeToClient);
      } else {
        sendMsgToClient(NO_MESSAGE, pipeToServer, pipeToClient);
      }
    } else {
      const std::string& musicToPlay = musicList->head_out()->get_music();
      ma_result result = Commands::play(pMa, musicToPlay, musicPlaying,
                                        musicList, quitC, config);
      if (result != MA_SUCCESS) {
        logErrorAndSendToClient(
            fmt::format("Failed to play music: {}", musicToPlay), pipeToServer,
            pipeToClient);
      } else {
        sendMsgToClient(NO_MESSAGE, pipeToServer, pipeToClient);
      }
    }
  } else if (subCmd == PLAY_LATER ||
             std::ranges::find(COMMANDS.at(PLAY_LATER), subCmd) !=
                 COMMANDS.at(PLAY_LATER).end()) {
    if (args.size() >= 2) {
      const std::string& musicToPlay = args.at(1);
      Commands::play_later(config, musicToPlay, musicList);
      sendMsgToClient(NO_MESSAGE, pipeToServer, pipeToClient);
    } else {
      logErrorAndSendToClient(
          fmt::format("{}: not enough arguments.", PLAY_LATER), pipeToServer,
          pipeToClient);
    }
  } else if (subCmd == PLAY_NEXT ||
             std::ranges::find(COMMANDS.at(PLAY_NEXT), subCmd) !=
                 COMMANDS.at(PLAY_NEXT).end()) {
    ma_result result = Commands::play_next(pMa);
    if (result != MA_SUCCESS) {
      logErrorAndSendToClient("Failed to play next music", pipeToServer,
                              pipeToClient);
    } else {
      sendMsgToClient(NO_MESSAGE, pipeToServer, pipeToClient);
    }
  } else if (subCmd == PLAY_PREV ||
             std::ranges::find(COMMANDS.at(PLAY_PREV), subCmd) !=
                 COMMANDS.at(PLAY_PREV).end()) {
    ma_result result = Commands::play_prev(pMa, musicList);
    if (result != MA_SUCCESS) {
      logErrorAndSendToClient("Failed to play prev music", pipeToServer,
                              pipeToClient);
    } else {
      sendMsgToClient(NO_MESSAGE, pipeToServer, pipeToClient);
    }
  } else if (subCmd == PAUSE || std::ranges::find(COMMANDS.at(PAUSE), subCmd) !=
                                    COMMANDS.at(PAUSE).end()) {
    ma_result result = Commands::pause(pMa);
    if (result != MA_SUCCESS) {
      logErrorAndSendToClient("Failed to pause/resume music", pipeToServer,
                              pipeToClient);
    } else {
      sendMsgToClient(NO_MESSAGE, pipeToServer, pipeToClient);
    }
  } else if (subCmd == CURSOR_FORWARD ||
             std::ranges::find(COMMANDS.at(CURSOR_FORWARD), subCmd) !=
                 COMMANDS.at(CURSOR_FORWARD).end()) {
    ma_result result = Commands::cursor_forward(pMa);
    if (result != MA_SUCCESS) {
      logErrorAndSendToClient("Failed to move cursor forward", pipeToServer,
                              pipeToClient);
    } else {
      sendMsgToClient(NO_MESSAGE, pipeToServer, pipeToClient);
    }
  } else if (subCmd == CURSOR_BACKWARD ||
             std::ranges::find(COMMANDS.at(CURSOR_BACKWARD), subCmd) !=
                 COMMANDS.at(CURSOR_BACKWARD).end()) {
    ma_result result = Commands::cursor_backward(pMa);
    if (result != MA_SUCCESS) {
      logErrorAndSendToClient("Failed to move cursor backward", pipeToServer,
                              pipeToClient);
    } else {
      sendMsgToClient(NO_MESSAGE, pipeToServer, pipeToClient);
    }
  } else if (subCmd == SET_CURSOR ||
             std::ranges::find(COMMANDS.at(SET_CURSOR), subCmd) !=
                 COMMANDS.at(SET_CURSOR).end()) {
    if (args.size() >= 2) {
      const std::string& time = args.at(1);
      ma_result result = Commands::set_cursor(pMa, time);
      if (result != MA_SUCCESS) {
        logErrorAndSendToClient("Failed to set cursor", pipeToServer,
                                pipeToClient);
      } else {
        sendMsgToClient(NO_MESSAGE, pipeToServer, pipeToClient);
      }
    }
  } else if (subCmd == GET_CURRENT_PROGRESS ||
             std::ranges::find(COMMANDS.at(GET_CURRENT_PROGRESS), subCmd) !=
                 COMMANDS.at(GET_CURRENT_PROGRESS).end()) {
    auto progress = std::make_unique<Progress>();
    ma_result result = Commands::get_current_progress(pMa, progress.get());
    if (result != MA_SUCCESS) {
      logErrorAndSendToClient("Failed to get progress", pipeToServer,
                              pipeToClient);
    } else {
      sendMsgToClient(progress->make_bar(), pipeToServer, pipeToClient);
    }
  } else if (subCmd == VOLUME_UP ||
             std::ranges::find(COMMANDS.at(VOLUME_UP), subCmd) !=
                 COMMANDS.at(VOLUME_UP).end()) {
    ma_result result = Commands::volume_up(pMa->pEngine.get());
    if (result != MA_SUCCESS) {
      logErrorAndSendToClient("Failed to turn the volume up", pipeToServer,
                              pipeToClient);
    } else {
      sendMsgToClient(NO_MESSAGE, pipeToServer, pipeToClient);
    }
  } else if (subCmd == VOLUME_DOWN ||
             std::ranges::find(COMMANDS.at(VOLUME_DOWN), subCmd) !=
                 COMMANDS.at(VOLUME_DOWN).end()) {
    ma_result result = Commands::volume_down(pMa->pEngine.get());
    if (result != MA_SUCCESS) {
      logErrorAndSendToClient("Failed to turn the volume down", pipeToServer,
                              pipeToClient);
    } else {
      sendMsgToClient(NO_MESSAGE, pipeToServer, pipeToClient);
    }
  } else if (subCmd == SET_VOLUME ||
             std::ranges::find(COMMANDS.at(SET_VOLUME), subCmd) !=
                 COMMANDS.at(SET_VOLUME).end()) {
    if (args.size() >= 2) {
      const std::string& volume = args.at(1);
      ma_result result = Commands::set_volume(pMa->pEngine.get(), volume);
      if (result != MA_SUCCESS) {
        logErrorAndSendToClient("Failed to set volume", pipeToServer,
                                pipeToClient);
      } else {
        sendMsgToClient(NO_MESSAGE, pipeToServer, pipeToClient);
      }
    } else {
      logErrorAndSendToClient(
          fmt::format("{}, not enough arguments.", SET_VOLUME), pipeToServer,
          pipeToClient);
    }
  } else if (subCmd == GET_VOLUME ||
             std::ranges::find(COMMANDS.at(GET_VOLUME), subCmd) !=
                 COMMANDS.at(GET_VOLUME).end()) {
    float volume = -1;
    ma_result result = Commands::get_volume(pMa->pEngine.get(), &volume);
    if (result != MA_SUCCESS) {
      logErrorAndSendToClient("Failed to get volume", pipeToServer,
                              pipeToClient);
    } else {
      sendMsgToClient(fmt::format("Volume: {}", volume), pipeToServer,
                      pipeToClient);
    }
  } else if (subCmd == MUTE || std::ranges::find(COMMANDS.at(MUTE), subCmd) !=
                                   COMMANDS.at(MUTE).end()) {
    ma_result result = Commands::mute(pMa->pEngine.get());
    if (result != MA_SUCCESS) {
      logErrorAndSendToClient("Failed to (un)mute", pipeToServer, pipeToClient);
    } else {
      sendMsgToClient(NO_MESSAGE, pipeToServer, pipeToClient);
    }
  } else if (subCmd == SET_RANDOM ||
             std::ranges::find(COMMANDS.at(SET_RANDOM), subCmd) !=
                 COMMANDS.at(SET_RANDOM).end()) {
    Commands::set_random(config);
    sendMsgToClient(
        fmt::format("random: {}", config->is_random() ? "enabled" : "disabled"),
        pipeToServer, pipeToClient);
  } else if (subCmd == SET_REPETITIVE ||
             std::ranges::find(COMMANDS.at(SET_REPETITIVE), subCmd) !=
                 COMMANDS.at(SET_REPETITIVE).end()) {
    Commands::set_repetitive(config);
    sendMsgToClient(
        fmt::format("repetitive: {}",
                    config->is_repetitive() ? "enabled" : "disabled"),
        pipeToServer, pipeToClient);
  } else if (subCmd == LIST || std::ranges::find(COMMANDS.at(LIST), subCmd) !=
                                   COMMANDS.at(LIST).end()) {
    std::vector<std::string> list = Commands::list(musicList);
    sendMsgToClient(&list, pipeToServer, pipeToClient);
  } else if (subCmd == QUIT || std::ranges::find(COMMANDS.at(QUIT), subCmd) !=
                                   COMMANDS.at(QUIT).end()) {
    Commands::quit(pMa, quitC);
    sendMsgToClient("Server quit successfully", pipeToServer, pipeToClient);
  }
}

void server() {
  // setup runtime
  auto pipeToServer = std::make_unique<NamedPipe>(PIPE_TO_SERVER);
  auto pipeToClient = std::make_unique<NamedPipe>(PIPE_TO_CLIENT);
  setup_runtime(pipeToServer.get(), pipeToClient.get());

  auto config = std::make_unique<Config>(REPETITIVE, RANDOM, USIC_LIBARY,
                                         PLAY_LISTS_PATH);
  auto pMa = std::make_unique<MaComponents>();
  pMa->ma_comp_init_engine();

  auto musicList = std::make_unique<MusicList>(
      config.get(),
      fmt::format("{}{}", config->get_playList_path(), DEFAULT_PLAY_LIST));

  if (musicList->is_empty()) {
    log(fmt::format("failed to load music list {}{}",
                    config->get_playList_path(), DEFAULT_PLAY_LIST),
        LogType::ERROR, __func__);
    std::exit(FATAL_ERROR);
  }

  auto musicPlaying = std::make_unique<std::string>();
  auto quitC = std::make_unique<QuitControl>();

  std::thread cleaner(cleanFunc, pMa.get(), quitC.get());
  cleaner.detach();

  std::string cmd;
  std::vector<std::string> args;
  while (!quitC->get_quit_flag()) {
    args.clear();
    cmd = get_command(pipeToServer.get(), pipeToClient.get());
    if (cmd == "") {
      log("failed to get command", LogType::ERROR, __func__);
      continue;
    }

    handle_command(cmd, pipeToServer.get(), pipeToClient.get(), pMa.get(),
                   musicPlaying.get(), musicList.get(), quitC.get(),
                   config.get());
  }
  pipeToServer->delete_pipe();
  pipeToClient->delete_pipe();
}