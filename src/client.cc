#include "client.h"

#include <string>

const std::string concatenateArgs(int argc, char* argv[]) {
  std::string result;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    // check if the argument contains spaces
    if (arg.find(' ') != std::string::npos) {
      result += "\"" + arg + "\"";  // add quotes
    } else {
      result += arg;
    }

    if (i < argc - 1) {  // add separator if not the last argument
      result += " ";
    }
  }

  return result;
}

void client(int argc, char* argv[]) {
  auto pipeToServer = std::make_unique<NamedPipe>(PIPE_TO_SERVER);
  auto pipeToClient = std::make_unique<NamedPipe>(PIPE_TO_CLIENT);

  pipeToServer->open_pipe(OpenMode::WR_ONLY_BLOCK);
  pipeToClient->open_pipe(OpenMode::RD_ONLY_BLOCK);

  // send command to server
  const std::string cmd = concatenateArgs(argc, argv);
  pipeToServer->writeIn(cmd);

  // receive response from server
  std::string msg;
  while (true) {
    msg = pipeToClient->readOut();
    if (msg != "") {
      if (msg == OVER) {
        break;
      }
      if (msg != NO_MESSAGE) {
        fmt::print("{}\n", msg);
      }
      pipeToServer->writeIn(GOT);
    }
  }
}