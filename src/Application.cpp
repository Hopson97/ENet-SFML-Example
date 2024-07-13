#include "Application.h"

#include <iostream>

#include <SFML/Network/Packet.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <imgui.h>

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
                return "Connecting...";
            case ConnectState::ConnectFailed:
                return "Connection failed";
        }
        return "Unknown";
    }
} // namespace

Application::Application(const sf::RenderWindow& window)
    : window_(window_)
{
}

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
                std::cerr << "Failed to create a client host.\n";
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
                std::cerr << "No available peers for initiating an ENet connection.\n";
                return;
            }

            // Await for success...
            ENetEvent event;
            if (enet_host_service(client_, &event, 5000) > 0 &&
                event.type == ENET_EVENT_TYPE_CONNECT)
            {
                connect_state_ = ConnectState::Connected;
                std::cout << "Connected!\n";
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
    keyboard_.update(e);
}

void Application::on_update(sf::Time dt)
{
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
                    case ToClientMessage::Message:
                    {
                        std::string message;
                        incoming_message.payload >> message;
                        std::cout << "Got message from server: " << message << '\n';
                    }
                    break;

                    case ToClientMessage::PlayerJoin:
                        std::cout << "A player has joined.\n";
                        break;

                    case ToClientMessage::PlayerLeave:
                        std::cout << "A player has left.\n";
                        break;

                    case ToClientMessage::Snapshot:
                    {
                        for (auto& player : entities_)
                        {
                            // Read state from the packet
                            int ticks;
                            sf::Vector2f position;
                            incoming_message.payload >> ticks >> player.id >> position.x >>
                                position.y >> player.active;

                            if (config_.do_interpolation)
                            {
                                player.position_buffer.push_back(
                                    {.timestamp = game_time_.getElapsedTime(),
                                     .position = position});
                            }
                            else
                            {
                                player.transform.position = position;
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

    Input inputs;

    constexpr float speed = 25;
    constexpr float MAX_SPEED = 256;
    if (keyboard_.isKeyDown(sf::Keyboard::W))
    {
        inputs.keys |= KeyPress::W;
        velocity_ += sf::Vector2f{0, -speed};
    }
    if (keyboard_.isKeyDown(sf::Keyboard::A))
    {
        inputs.keys |= KeyPress::A;
        velocity_ += {-speed, 0};
    }
    if (keyboard_.isKeyDown(sf::Keyboard::S))
    {
        inputs.keys |= KeyPress::S;
        velocity_ += {0, speed};
    }
    if (keyboard_.isKeyDown(sf::Keyboard::D))
    {
        inputs.keys |= KeyPress::D;
        velocity_ += {speed, 0};
    }

    
    velocity_.x = std::clamp(velocity_.x, -MAX_SPEED, MAX_SPEED) * 0.94f;
    velocity_.y = std::clamp(velocity_.y, -MAX_SPEED, MAX_SPEED) * 0.94f;
    position_ += velocity_ * dt.asSeconds();

    // Send position
    ToServerNetworkMessage position_message(ToServerMessageType::Position);
    position_message.payload << position_.x << position_.y;
    enet_peer_send(peer_, 0, position_message.to_enet_packet((ENetPacketFlag)0));
    

    
    // Deubgging t
    static std::vector<float> rts;
    static std::vector<float> ts;
    static std::vector<float> t0s;
    static std::vector<float> t1s;

    if (config_.do_interpolation)
    {
        auto now = game_time_.getElapsedTime();
        auto render_ts = (now - sf::milliseconds(1000.0f / SERVER_TPS) * 4.0f);
        for (auto& entity : entities_)
        {
            if (!entity.active || entity.position_buffer.size() < 2)
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

                entity.transform.position = {nx, ny};
            }
        }

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
    }
}

void Application::on_fixed_update(sf::Time dt)
{

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
        }
    }
    ImGui::End();

    if (connect_state_ != ConnectState::Connected)
    {
        return;
    }

    // Draw player
    sprite_.setPosition(position_);
    sprite_.setFillColor(sf::Color::Red);
    sprite_.setSize({32, 32});
    window.draw(sprite_);

    // Draw entities
    sprite_.setFillColor({255, 0, 255, 100});
    for (auto& e : entities_)
    {
        if (e.active)
        {
            sprite_.setPosition(e.transform.position);
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
                    std::cout << "Disconnect success\n";
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
