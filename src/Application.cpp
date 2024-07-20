#include "Application.h"

#include <iostream>

#include <SFML/Network/Packet.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <cmath>
#include <imgui.h>
#include <print>

#include "NetworkMessage.h"
#include "Util/Util.h"

namespace
{
    const char* connect_state_to_string(ConnectState state)
    {
        switch (state)
        {
            case ConnectState::Disconnected:
                return "Disconnected";
            case ConnectState::Connected:
                return "Connected";
            case ConnectState::Connecting:
                return "Connecting..";
            case ConnectState::ConnectFailed:
                return "Connection failed";
        }
        return "Unknown";
    }
} // namespace

Application::~Application()
{
    disconnect();
}

bool Application::init_as_host()
{
    if (!server_.run())
    {
        return false;
    }
    return init_as_client();
}

bool Application::init_as_client()
{
    connect_thread_ = std::jthread(
        [&]
        {
            connect_state_ = ConnectState::Connecting;

            client_ = enet_host_create(nullptr, 1, 2, 0, 0);
            if (!client_)
            {
                connect_state_ = ConnectState::ConnectFailed;
                std::println(std::cerr, "Failed to create a client host.");
                return;
            }

            ENetAddress address{};
            enet_address_set_host(&address, "127.0.0.1");
            address.port = 12345;

            // Connect!
            peer_ = enet_host_connect(client_, &address, 2, 0);
            if (!peer_)
            {
                connect_state_ = ConnectState::ConnectFailed;
                std::println(std::cerr, "No available peers for initiating an ENet connection.");
                return;
            }

            // Await for success..
            ENetEvent event;
            if (enet_host_service(client_, &event, 5000) > 0 &&
                event.type == ENET_EVENT_TYPE_CONNECT)
            {
                connect_state_ = ConnectState::Connected;
                std::println("[Client] Connected!");
            }
            else
            {
                enet_peer_reset(peer_);
                connect_state_ = ConnectState::ConnectFailed;
                std::cerr << "Connection to server failed.\n";
                return;
            }
        });

    return true;
}

void Application::on_event([[maybe_unused]] const sf::RenderWindow& window, const sf::Event& e)
{
    if (window.hasFocus())
    {

        keyboard_.update(e);
    }
    else
    {
        keyboard_.reset();
        std::println("reset keyboardzn");
    }
}

void Application::on_update(sf::Time dt)
{
    // Each update is done as follows and is important to be done in this order:
    //
    // 1. Handle incoming packets from the server
    // 2. Handle input from keyboard, store in buffer
    // 3. Send input to the server
    // 4. Apply client side prediction (From 4)
    // 5. Apply snapshot interpolation (From 1)

    if (connect_state_ != ConnectState::Connected)
    {
        return;
    }

    for (ENetEvent event; enet_host_service(client_, &event, 0);)
    {
        switch (event.type)
        {
            case ENET_EVENT_TYPE_RECEIVE:
            {
                ToClientNetworkMessage incoming_message(event.packet);
                switch (incoming_message.message_type)
                {
                    case ToClientMessage::ClientInfo:
                        incoming_message.payload >> player_id_;
                        break;

                    case ToClientMessage::Message:
                    {
                        std::string message;
                        incoming_message.payload >> message;
                        std::println("[Client]  Got message from server: {}", message);
                    }
                    break;

                    case ToClientMessage::PlayerJoin:
                        std::println("[Client] A player has joined.\n");
                        break;

                    case ToClientMessage::PlayerLeave:
                        std::println("[Client] A player has left.\n");
                        break;

                    // Snapshot contains the positons of ALL entities - including the player's own
                    case ToClientMessage::Snapshot:
                    {
                        // In this example, the server and client should have matching arrays that
                        // align with what is in the packet
                        for (auto& entity : entities_)
                        {
                            // Read state from the packet - TODO seperate this logic out from the
                            // looping of players
                            sf::Vector2f position;
                            u32 input_sequence = 0;
                            incoming_message.payload >> entity.common.id >> input_sequence >>
                                position.x >> position.y >> entity.common.active;

                            // If the entity is "this player"
                            if (entity.common.id == player_id_)
                            {
                                // Set position
                                entities_[(size_t)player_id_].common.transform.position = position;

                                // Correct position hen the server is out of sync with this client
                                if (config_.server_reconciliation_)
                                {
                                    bool out_of_sync_found = false;
                                    for (const auto& pending_input : pending_inputs_)
                                    {
                                        if (pending_input.input.sequence > input_sequence)
                                        {
                                            // When an out-of-sync input is found, the player state
                                            // is reset back to that to ensure the final result is
                                            // identical after re-applping the inputs
                                            if (!out_of_sync_found)
                                            {
                                                entities_[(size_t)player_id_].common.transform =
                                                    pending_input.state;
                                                out_of_sync_found = true;
                                            }
                                            process_input_for_player(
                                                entities_[(size_t)player_id_].common.transform,
                                                pending_input.input);
                                        }
                                    }
                                }
                                pending_inputs_.clear();
                            }
                            else if (entity.common.active)
                            {
                                if (config_.do_interpolation)
                                {
                                    entity.position_buffer.push_back(
                                        {.timestamp = game_time_.getElapsedTime(),
                                         .position = position});
                                }
                                else
                                {
                                    entity.common.transform.position = position;
                                }
                            }
                        }
                    }
                    break;

                    default:
                        break;
                }
                enet_packet_destroy(event.packet);
            }
            break;

            default:
                break;
        }
    }

    // Process the inputs, storing the key presses into an object to be sent to the server
    Input inputs{.sequence = input_sequence_++, .dt = dt.asSeconds()};
    if (keyboard_.is_key_down(sf::Keyboard::W))
    {
        inputs.keys |= InputKeyPress::W;
    }
    if (keyboard_.is_key_down(sf::Keyboard::A))
    {
        inputs.keys |= InputKeyPress::A;
    }
    if (keyboard_.is_key_down(sf::Keyboard::S))
    {
        inputs.keys |= InputKeyPress::S;
    }
    if (keyboard_.is_key_down(sf::Keyboard::D))
    {
        inputs.keys |= InputKeyPress::D;
    }

    // Send the input packet to the server
    ToServerNetworkMessage input_message(ToServerMessageType::Input);
    input_message.payload << inputs.sequence << inputs.dt << inputs.keys;
    enet_peer_send(peer_, 0, input_message.to_enet_packet());

    pending_inputs_.push_back({inputs, entities_[(size_t)player_id_].common.transform});

    // Client side prediction ensures the player sees smooth movement despite the real
    // simulation being om the server
    // Without this, the player position is delayed and jittered as it must wait for the server to
    // process the input
    if (config_.client_side_prediction_)
    {
        process_input_for_player(entities_[(size_t)player_id_].common.transform, inputs);
    }

    // Deubgging t
    static std::vector<float> rts;
    static std::vector<float> ts;
    static std::vector<float> t0s;
    static std::vector<float> t1s;

    if (config_.do_interpolation)
    {
        auto now = game_time_.getElapsedTime();
        auto render_ts = (now - sf::milliseconds(1000.0f / (float)SERVER_TPS) * 4.0f);
        for (auto& entity : entities_)
        {
            if (!entity.common.active || entity.position_buffer.size() < 2 ||
                player_id_ == entity.common.id)
            {
                continue;
            }
            auto& buffer = entity.position_buffer;

            while (buffer.size() > 2 && buffer[1].timestamp <= render_ts)
            {
                buffer.erase(buffer.begin());
            }

            const auto t0 = buffer[0].timestamp;
            const auto t1 = buffer[1].timestamp;

            if (t0 <= render_ts && render_ts <= t1)
            {
                const auto& p0 = buffer[0].position;
                const auto& p1 = buffer[1].position;

                auto t = ((render_ts - t0) / (t1 - t0));
                float nx = std::lerp(p0.x, p1.x, t);
                float ny = std::lerp(p0.y, p1.y, t);

                // Debug t
                ts.push_back(t);
                t0s.push_back(t0.asSeconds());
                t1s.push_back(t1.asSeconds());
                rts.push_back(render_ts.asSeconds());

                entity.common.transform.position = {nx, ny};
            }
        }
        /*
        while (ts.size() > 10)
        {
            ts.erase(ts.begin());
        }
        while (rts.size() > 10)
        {
            rts.erase(rts.begin());
        }
        while (t0s.size() > 10)
        {
            t0s.erase(t0s.begin());
        }
        while (t1s.size() > 10)
        {
            t1s.erase(t1s.begin());
        }

        if (ImGui::Begin("Times."))
        {
            if (ImGui::BeginTable("split", 4))
            {
                ImGui::TableNextColumn();
                ImGui::Text("T");
                for (auto t : ts)
                {
                    ImGui::Text("%f", t);
                }

                ImGui::TableNextColumn();
                ImGui::Text("T0");
                for (auto t : t0s)
                {
                    ImGui::Text("%f", t);
                }

                ImGui::TableNextColumn();
                ImGui::Text("T1");
                for (auto t : t1s)
                {
                    ImGui::Text("%f", t);
                }

                ImGui::TableNextColumn();
                ImGui::Text("Render Timestamp");
                for (auto t : rts)
                {
                    ImGui::Text("%f", t);
                }

                ImGui::EndTable();
            }
        }
        ImGui::End();
    */
    }
}

void Application::on_render(sf::RenderWindow& window)
{
    static char message[128];
    if (ImGui::Begin("Connect status + Debug"))
    {
        ImGui::Text("%s", connect_state_to_string(connect_state_));

        if (connect_state_ == ConnectState::Connected)
        {
            ImGui::InputText("Message", message, 128);

            if (ImGui::Button("Send something"))
            {
                ToServerNetworkMessage outgoing_message{ToServerMessageType::Message};
                outgoing_message.payload << std::string{message};
                enet_peer_send(peer_, 0, outgoing_message.to_enet_packet());
                enet_host_flush(client_);
            }

            ImGui::Separator();
            ImGui::Text("Config Options");
            ImGui::Checkbox("Interpolation: ", &config_.do_interpolation);
            ImGui::Checkbox("Client Side Prediction: ", &config_.client_side_prediction_);
            ImGui::Checkbox("Server Reconciliation: ", &config_.server_reconciliation_);
        }
    }
    ImGui::End();

    if (connect_state_ != ConnectState::Connected)
    {
        return;
    }

    // Draw tiles
    sprite_.setOutlineThickness(1);
    sprite_.setSize({TILE_SIZE, TILE_SIZE});
    for (int y = 0; y < MAP_SIZE; y++)
    {
        for (int x = 0; x < MAP_SIZE; x++)
        {
            auto tile = get_tile(x, y);
            if (tile > 0)
            {
                if (tile == 1)
                {
                    sprite_.setFillColor(sf::Color::Green);
                }
                sprite_.setPosition(x * TILE_SIZE, y * TILE_SIZE);
                window.draw(sprite_);
            }
        }
    }
    sprite_.setOutlineThickness(0);

    // Draw player
    sprite_.setSize(entities_[(size_t)player_id_].common.transform.size);
    sprite_.setPosition(entities_[(size_t)player_id_].common.transform.position);
    sprite_.setFillColor(sf::Color::Red);
    window.draw(sprite_);

    // Draw entities
    sprite_.setFillColor({255, 0, 255, 100});
    for (auto& e : entities_)
    {

        if (e.common.id != player_id_ && e.common.active)
        {
            sprite_.setSize(e.common.transform.size);
            // Non-player entities are shown as a different colour
            if (e.common.id >= MAX_CLIENTS)
            {
                sprite_.setFillColor({255, 255, 150, 100});
            }

            sprite_.setPosition(e.common.transform.position);
            window.draw(sprite_);
        }
    }
}

void Application::disconnect()
{
    if (peer_)
    {
        enet_peer_disconnect(peer_, 0);

        // Wait for the disconnect to complete.
        ENetEvent event = {};
        bool success_disconnect = false;
        while (enet_host_service(client_, &event, 2000) > 0)
        {
            switch (event.type)
            {
                case ENET_EVENT_TYPE_RECEIVE:
                    enet_packet_destroy(event.packet);
                    break;

                case ENET_EVENT_TYPE_DISCONNECT:

                    std::println("[Client] Disconnect success");
                    success_disconnect = true;
                    break;

                default:
                    break;
            }
        }
        if (!success_disconnect)
        {
            enet_peer_reset(peer_);
        }
    }
    enet_host_destroy(client_);
}
