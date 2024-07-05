#include "Application.h"

#include <iostream>

#include <SFML/Network/Packet.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <imgui.h>

#include "NetworkMessage.h"

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

Application::Application()
{
    sprite_.setFillColor(sf::Color::Red);
    sprite_.setSize({64, 64});
    for (auto& e : entities_)
    {
        e.sprite.setFillColor(sf::Color::Blue);
        e.sprite.setSize({64, 64});
    }
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

            ENetAddress address = {0};
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

void Application::on_event(const sf::RenderWindow& window, const sf::Event& e)
{
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
                        std::cout << "Got message from server:" << message << '\n';
                    }
                    break;

                    case ToClientMessage::PlayerJoin:
                        std::cout << "A player has joined.\n";
                        break;

                    case ToClientMessage::PlayerLeave:
                        std::cout << "A player has left.\n";
                        break;

                    default:
                        break;
                }
                enet_packet_destroy(event.packet);
            }
            break;

            case ENET_EVENT_TYPE_DISCONNECT:
                std::cout << std::format("Client has disconnected: {}.\n", 1);
                /* Reset the peer's client information. */
                event.peer->data = nullptr;
                break;

            case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
                std::cout << std::format("Client has timeouted: {}.\n", 1);
                /* Reset the peer's client information. */
                event.peer->data = nullptr;
                break;

            default:
                break;
        }
    }

    // Send position


    constexpr int speed = 8;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
    {
        sprite_.move({0, -1});
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
    {
        sprite_.move({-1, 0});
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
    {
        sprite_.move({0, 1});
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
    {
        sprite_.move({1, 0});
    }
}

void Application::on_fixed_update(sf::Time dt)
{
}

void Application::on_render(sf::RenderWindow& window)
{
    static char message[128];
    if (ImGui::Begin("Connect status."))
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
        }
    }
    ImGui::End();

    if (connect_state_ != ConnectState::Connected)
    {
        return;
    }

    window.draw(sprite_);
    for (auto& e : entities_)
    {
        if (e.active)
        {
            window.draw(e.sprite);
        }
    }
}

void Application::disconnect()
{
    if (peer_)
    {
        std::cout << "Disconnecting\n";
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
            }
        }
        if (!success_disconnect)
        {
            enet_peer_reset(peer_);
        }
    }
    enet_host_destroy(client_);
}
