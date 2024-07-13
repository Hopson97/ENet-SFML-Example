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
    Ready,
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

    EntityCommon common;
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

    /// If this client is the host, then the server is created on a different thread
    Server server_;

    ENetHost* client_ = nullptr;
    ENetPeer* peer_ = nullptr;

    /// Connecting this client to the server is done via a state machine - on a different thread to avoid freezing the 
    /// window while connecting
    std::atomic<ConnectState> connect_state_ = ConnectState::Disconnected;
    std::jthread connect_thread_;

    /// Used to render all players and entities
    sf::RectangleShape sprite_;

    /// The client Id of this player - used to index the `entities_` array
    i16 player_id_ = 0;

    /// All entities
    std::array<Entity, MAX_ENTITIES> entities_;

    u32 input_sequence_ = 0;
    Keyboard keyboard_;

    struct Config
    {
        bool do_interpolation = true;
        bool client_side_prediction_ = true;
    } config_;

    sf::Clock game_time_;
};