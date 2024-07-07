#pragma once

#include <atomic>
#include <thread>
#include <array>

#include <enet/enet.h>
#include <SFML/System/Vector2.hpp>
#include <SFML/System/Time.hpp>


constexpr int MAX_CLIENTS = 5;

struct ServerPlayer
{
    int id = -1;
    ENetPeer* peer = nullptr;
    sf::Vector2f position;
};

struct Entity2
{
    struct PositionBuffer
    {
        sf::Time timestamp;
        sf::Vector2f position;
    };

    std::array<PositionBuffer, 2> position_buffer;
};

class Server
{
  public:
    Server();
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

    std::array<ServerPlayer, MAX_CLIENTS> players_;
};