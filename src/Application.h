#pragma once

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>

#include "Common.h"
#include "Keyboard.h"
#include "Server.h"

enum class ConnectState
{
    Disconnected,
    Connecting,
    Connected,
    ConnectFailed,
};

struct Entity
{
    struct PositionBuffer
    {
        sf::Time timestamp;
        sf::Vector2f position;
    };
    std::vector<PositionBuffer> position_buffer;

    EntityTransform transform;
    int id = -1;
    bool active = false;
};

class Application
{
  public:
    Application(const sf::RenderWindow& window);
    ~Application();

    bool init_as_host();
    bool init_as_client();

    void on_event(const sf::RenderWindow& window, const sf::Event& e);
    void on_update(sf::Time dt);
    void on_fixed_update(sf::Time dt);
    void on_render(sf::RenderWindow& window);
    void disconnect();

  private:
    const sf::RenderWindow& window_;
    Server server_;

    ENetHost* client_ = nullptr;
    ENetPeer* peer_ = nullptr;

    std::atomic<ConnectState> connect_state_ = ConnectState::Disconnected;
    std::jthread connect_thread_;

    sf::RectangleShape sprite_;
    sf::Vector2f position_;
    sf::Vector2f velocity_;

    std::array<Entity, MAX_CLIENTS> entities_;

    Keyboard keyboard_;

    struct Config
    {
        bool do_interpolation = true;
    } config_;

    sf::Clock game_time_;
};