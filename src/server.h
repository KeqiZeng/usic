#pragma once

#include "named_pipe.h"
#include <vector>

class Server
{
  public:
    Server();
    ~Server()                        = default;
    Server(const Server&)            = delete;
    Server(Server&&)                 = delete;
    Server& operator=(const Server&) = delete;
    Server& operator=(Server&&)      = delete;

    void run();

    static Server& getInstance() noexcept;

  private:
    void setupRuntime();
    std::string getMessageFromClient() noexcept;
    void sendMsgToClient(std::string_view msg) noexcept;
    void sendMsgToClient(const std::vector<std::string>* msg);
    void logErrorAndSendToClient(std::string_view err);
    void handleCommand(const std::string& cmd);

    NamedPipe pipe_to_client_;
    NamedPipe pipe_from_client_;
};
