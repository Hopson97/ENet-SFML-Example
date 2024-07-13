#include "Server.h"

#include <iostream>

#include "NetworkMessage.h"

#include "Util/Util.h"

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

Server::Server()
{
    for (int i = 0; i < players_.size(); i++)
    {
        players_[i].id = i;
    }
}

Server::~Server()
{
    stop();
}

bool Server::run()
{
    ENetAddress address = {.host = ENET_HOST_ANY, .port = 12345, .sin6_scope_id = 0};
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
        std::this_thread::sleep_for(std::chrono::milliseconds((int)SERVER_TPS));

        if (++ticks % 20 == 0)
        {
            std::cout << "[Server] Ticks: " << ticks << " (" << ticks / 20 << " seconds)" << '\n';
        }

        ENetEvent event;
        while (enet_host_service(server_, &event, 0) > 0)
        {
            switch (event.type)
            {
                case ENET_EVENT_TYPE_CONNECT:
                {

                    // Host -> event.peer->address.host
                    // Port -> event.peer->address.port
                    std::cout << "[Server] A new client connected.\n";
                    i16 id = 0;
                    for (auto& player : players_)
                    {
                        std::cout << "[Server] Finding a slot...\n";
                        if (!player.peer)
                        {
                            id = player.id;
                            player.peer = event.peer;
                            event.peer->data = (void*)&player;
                            std::cout << "[Server] Slot: " << (int)player.id << '\n';
                            id = player.id;
                            break;
                        }
                    }
                    std::cout << id << std::endl;
                    ToClientNetworkMessage client_id{ToClientMessage::ClientInfo};
                    client_id.payload << id;
                    enet_peer_send(event.peer, 0, client_id.to_enet_packet());

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
                            std::cout << "[Server] Got message from client: " << text << '\n';

                            ToClientNetworkMessage outgoing_message{ToClientMessage::Message};
                            outgoing_message.payload << text;
                            enet_host_broadcast(server_, 0, outgoing_message.to_enet_packet());
                            enet_host_flush(server_);
                        }
                        break;

                        case ToServerMessageType::Input:
                        {
                            Input input;
                            u32 sequence = 0;
                            auto player = (ServerPlayer*)event.peer->data;

                            incoming_message.payload >> sequence >> input.dt >> input.keys;

                            process_input_for_player(player->transform, input);

                            // incoming_message.payload >> player->transform.position.x >>
                            //     player->transform.position.y;
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

                    std::cout << "[Server] Client has disconnected.\n";
                    reset_player_peer(event.peer);
                    ToClientNetworkMessage outgoing_message{ToClientMessage::PlayerLeave};
                    enet_host_broadcast(server_, 0, outgoing_message.to_enet_packet());
                }
                break;

                case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
                {
                    std::cout << "[Server] Client has timed-out.\n";
                    reset_player_peer(event.peer);
                    ToClientNetworkMessage outgoing_message{ToClientMessage::PlayerLeave};
                    enet_host_broadcast(server_, 0, outgoing_message.to_enet_packet());
                }
                break;

                default:
                    break;
            }
        }

        ToClientNetworkMessage snapshot(ToClientMessage::Snapshot);
        for (const auto& player : players_)
        {
            snapshot.payload << player.id << player.transform.position.x
                             << player.transform.position.y << (player.peer ? true : false);
        }
        enet_host_broadcast(server_, 0, snapshot.to_enet_packet());
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
