#include "Server.h"

#include <iostream>

#include <SFML/Network/Packet.hpp>

constexpr int MAX_CLIENTS = 5;

Server::~Server()
{
    stop();
}

bool Server::run()
{
    ENetAddress address = {.host = ENET_HOST_ANY, .port = 12345};
    server_ = enet_host_create(&address, MAX_CLIENTS, 2, 0, 0);

    if (!server_)
    {
        printf("An error occurred while trying to create an ENet server host.\n");
        return false;
    }

    server_thread_ = std::jthread([&] { launch(); });
    return true;
}

void Server::launch()
{
    running_ = true;
    while (running_)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "Hello world\n";

        ENetEvent event;
        while (enet_host_service(server_, &event, 0) > 0)
        {
            std::cout << "Got event\n";
            switch (event.type)
            {
                case ENET_EVENT_TYPE_CONNECT:
                    // Host -> event.peer->address.host
                    // Port -> event.peer->address.port
                    std::cout << "A new client connected.\n";
                    /* Store any relevant client information here. */
                    event.peer->data = (void*)5;
                    break;

                case ENET_EVENT_TYPE_RECEIVE:
                {
                    sf::Packet p;
                    p.append(event.packet->data, event.packet->dataLength);

                    std::string message;
                    p >> message;

                    std::cout << "Got message from client:" << message << '\n';

                    sf::Packet sfml_packet;
                    sfml_packet << message;
                    ENetPacket* packet =
                        enet_packet_create(sfml_packet.getData(), sfml_packet.getDataSize(), 0);

                    for (int i = 0; i < server_->connectedPeers; i++)
                    {
                        enet_peer_send(&server_->peers[i], 0, packet);
                    }
                    enet_host_flush(server_);

                    /* Clean up the packet now that we're done using it. */
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
    }
}

void Server::stop()
{
    // enet_host_destroy(server_);
    if (running_)
    {
        running_ = false;
        server_thread_.join();
    }
}
