#include "Application.h"

#include <iostream>

Application::Application()
{
}

Application::~Application()
{
    if (peer_)
    {
        enet_peer_reset(peer_);
    }
    enet_host_destroy(client_);
}

bool Application::on_start()
{
    if (!server_.run())
    {
        return false;
    }

    client_ = enet_host_create(nullptr, 1, 2, 0, 0);
    if (!client_)
    {
        std::cerr << "Failed to create a client host.\n";
        return false;
    }

    ENetAddress address = {0};
    enet_address_set_host(&address, "127.0.0.1");
    address.port = 12345;

    // Connect!
    ENetPeer* peer = enet_host_connect(client_, &address, 2, 0);
    if (!peer)
    {
        std::cerr << "No available peers for initiating an ENet connection.\n";
        return false;
    }

    // Await for success...
    ENetEvent event;
    if (enet_host_service(client_, &event, 5000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT)
    {
        std::cout << "Connected!\n";
    }
    else
    {
        enet_peer_reset(peer);
        std::cerr << "Connection to server failed.\n";
        return false;
    }

    return true;
}

void Application::on_event(const sf::RenderWindow& window, const sf::Event& e)
{
}

void Application::on_update(sf::Time dt)
{
    for (ENetEvent event; enet_host_service(client_, &event, 0);)
    {
        switch (event.type)
        {
            case ENET_EVENT_TYPE_RECEIVE:
                printf("A packet of length %lu containing %s was received from %s on channel %u.\n",
                       event.packet->dataLength, event.packet->data, event.peer->data,
                       event.channelID);
                /* Clean up the packet now that we're done using it. */
                enet_packet_destroy(event.packet);
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
}

void Application::on_fixed_update(sf::Time dt)
{
}

void Application::on_render(sf::RenderWindow& window)
{
}
