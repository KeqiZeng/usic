#pragma once
#include "named_pipe.h"

class Client
{
  public:
    Client();
    ~Client()                        = default;
    Client(const Client&)            = delete;
    Client(Client&&)                 = delete;
    Client& operator=(const Client&) = delete;
    Client& operator=(Client&&)      = delete;

    void run(int argc, char** argv);
    static Client& getInstance() noexcept;

  private:
    NamedPipe pipe_to_client_;
    NamedPipe pipe_from_client_;
};
