#include "Server.h"

#include <iostream>

#include "NetworkMessage.h"



namespace
{
    void reset_player_peer(ENetPeer* peer)
    {
        if (!peer)
        {
            return;
        }

        auto player = (ServerPlayer*)peer->data;
        if (player)
        {
            player->peer = nullptr;
        }
        peer->data = nullptr;
    }
} // namespace

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
    int ticks = 0;
    running_ = true;
    while (running_)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        if (++ticks % 20 == 0)
        {
            std::cout << "Ticks: " << ticks << " (" << ticks / 20 << " seconds)" << '\n';
        }

        ENetEvent event;
        while (enet_host_service(server_, &event, 0) > 0)
        {
            std::cout << "Got event\n";
            switch (event.type)
            {
                case ENET_EVENT_TYPE_CONNECT:
                {

                    // Host -> event.peer->address.host
                    // Port -> event.peer->address.port
                    std::cout << "A new client connected.\n";
                    for (auto& player : players_)
                    {
                        if (!player.peer)
                        {
                            player.peer = event.peer;
                            event.peer->data = (void*)&player;
                        }
                    }
                    ToClientNetworkMessage outgoing_message{ToClientMessage::PlayerJoin};
                    enet_host_broadcast(server_, 0, outgoing_message.to_enet_packet());
                }
                break;

                case ENET_EVENT_TYPE_RECEIVE:
                {
                    ToServerNetworkMessage incoming_message{event.packet};
                    switch (incoming_message.message_type)
                    {
                        case ToServerMessageType::Message:
                        {
                            std::string text;
                            incoming_message.payload >> text;
                            std::cout << "Got message from client: " << text << '\n';

                            ToClientNetworkMessage outgoing_message{ToClientMessage::Message};
                            outgoing_message.payload << text;
                            enet_host_broadcast(server_, 0, outgoing_message.to_enet_packet());
                            enet_host_flush(server_);
                        }
                        break;

                        default:
                            break;
                    }
                    enet_packet_destroy(event.packet);
                }
                break;

                case ENET_EVENT_TYPE_DISCONNECT:
                {

                    std::cout << std::format("Client has disconnected: {}.\n", 1);
                    reset_player_peer(event.peer);
                    ToClientNetworkMessage outgoing_message{ToClientMessage::PlayerLeave};
                    enet_host_broadcast(server_, 0, outgoing_message.to_enet_packet());
                }
                break;

                case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
                {
                    std::cout << std::format("Client has timeouted: {}.\n", 1);
                    reset_player_peer(event.peer);
                    ToClientNetworkMessage outgoing_message{ToClientMessage::PlayerLeave};
                    enet_host_broadcast(server_, 0, outgoing_message.to_enet_packet());
                }
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
