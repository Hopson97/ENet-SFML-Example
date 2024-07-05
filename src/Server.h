#pragma once

#include <atomic>
#include <thread>
#include <array>

#include <SFML/System/Vector2.hpp>

#include <enet/enet.h>

struct ServerPlayer
{
    ENetPeer* peer = nullptr;
    sf::Vector2f position;
};

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

    std::array<ServerPlayer, 4> players_;
};