#include "Server.h"

#include <iostream>

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
                    std::printf("A new client connected from %x:%u.\n", event.peer->address.host,
                                event.peer->address.port);
                    /* Store any relevant client information here. */
                    event.peer->data = (void*)5;
                    break;

                case ENET_EVENT_TYPE_RECEIVE:
                    printf("A packet of length %lu containing %s was received from %s on channel "
                           "%u.\n",
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
