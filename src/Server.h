#pragma once

#include <atomic>
#include <memory>
#include <thread>

#include <enet/enet.h>

class Server
{
  public:
    Server() = default;
    ~Server();

    Server operator=(const Server& server) = delete;
    Server operator=(Server&& server) = delete;
    Server(Server&& server) = delete;
    Server(Server& server) = delete;

    [[nodiscard]] bool run();
    void stop();

  private:
    void launch();


    std::jthread server_thread_;
    std::atomic_bool running_ = false;

    // Network stuff
    ENetHost* server_ = nullptr;
};