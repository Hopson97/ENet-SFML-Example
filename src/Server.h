#pragma once

#include <atomic>
#include <thread>
#include <array>

#include <enet/enet.h>
#include <SFML/System/Vector2.hpp>
#include <SFML/System/Time.hpp>

#include "Common.h"


constexpr int MAX_CLIENTS = 4;
constexpr int MAX_ENTITIES = 5;
constexpr float SERVER_TICK_RATE = 20;
constexpr float SERVER_TPS = 1000 / SERVER_TICK_RATE;

struct ServerEntity
{
    i16 id = -1;
    ENetPeer* peer = nullptr; 
    EntityCommon common;

    u32 last_processed = 0;
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

    ENetHost* server_ = nullptr;

    std::vector<ServerEntity> entities_{MAX_ENTITIES};
};