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
    /// The position buffer is used for client side interpolation
    struct PositionBuffer
    {
        sf::Time timestamp;
        sf::Vector2f position;
    };
    std::vector<PositionBuffer> position_buffer;

    /// Transforms, id, etc
    EntityCommon common;
};

/// Contains the input state, and the player's own transform at the time the input was received
struct InputBuffer
{
    Input input;
    EntityTransform state;
};

// The client application
class Application
{
  public:
    ~Application();

    /// Creates a server on a background thread and connects to it 
    bool init_as_host();

    /// Only connect to an already running server
    bool init_as_client();

    void on_event(const sf::RenderWindow& window, const sf::Event& e);
    void on_update(sf::Time dt);
    void on_render(sf::RenderWindow& window);
    void disconnect();

  private:
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
    std::vector<Entity> entities_{MAX_ENTITIES};

    /// Used
    u32 input_sequence_ = 0;
    std::vector<InputBuffer> pending_inputs_;


    Keyboard keyboard_;

    struct Config
    {
        bool do_interpolation = true;
        bool client_side_prediction_ = true;
        bool server_reconciliation_ = true;
    } config_;

    sf::Clock game_time_;
};