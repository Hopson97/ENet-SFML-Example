#pragma once

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>

#include "Server.h"


enum class ConnectState
{
    Disconnected,
    ConnectedSuccess,
    ConnectFailed,
    Connected
};

class Application
{
  public:
    Application();
    ~Application();

    [[nodiscard]] bool on_start();

    void on_event(const sf::RenderWindow& window, const sf::Event& e);
    void on_update(sf::Time dt);
    void on_fixed_update(sf::Time dt);
    void on_render(sf::RenderWindow& window);

  private:
    Server server_;

    ENetHost* client_ = nullptr;
    ENetPeer* peer_ = nullptr;

    std::atomic<ConnectState> connect_state_ = ConnectState::Disconnected;
    std::jthread connect_thread_;
};