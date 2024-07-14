#include "Server.h"

#include <iostream>
#include <ranges>

#include "NetworkMessage.h"

#include "Util/Util.h"

namespace
{
    auto reset_player_peer(ENetPeer* peer)
    {
        if (!peer)
        {
            return -1;
        }

        auto player = (ServerPlayer*)peer->data;
        if (player)
        {
            player->peer = nullptr;
            player->entity_id = -1;
        }
        peer->data = nullptr;
        return player->entity_id;
    }
} // namespace

Server::Server()
{
    // entities_.reserve(128);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        auto& e = entities_.emplace_back();
        e.common.id = i;
    }

    for (int i = 0; i < 5; i++)
    {
        auto& e = entities_.emplace_back();
        e.common.active = true;
        e.common.id = static_cast<i16>(entities_.size() - 1);
        e.common.transform.position = {(float)(rand() % 1000), (float)(rand() % 1000)};
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
                    for (int i = 0; i < MAX_CLIENTS; i++)
                    {
                        std::cout << "[Server] Finding a slot...\n";
                        if (!players_[i].peer)
                        {
                            id = entities_[i].common.id;
                            entities_[i].common.active = true;
                            players_[i].peer = event.peer;
                            players_[i].entity_id = id;
                            event.peer->data = (void*)&players_[i];
                            std::cout << "[Server] Slot: " << (int)entities_[i].common.id << '\n';
                            break;
                        }
                    }
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
                            auto player = (ServerPlayer*)event.peer->data;
                            auto& entity = entities_[player->entity_id];

                            incoming_message.payload >> entity.last_processed >> input.dt >>
                                input.keys;

                            process_input_for_player(entity.common.transform, input);
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
                    auto entity_id = reset_player_peer(event.peer);
                    if (entity_id >= 0)
                    {
                        entities_[entity_id].common.active = false;
                    }

                    ToClientNetworkMessage outgoing_message{ToClientMessage::PlayerLeave};
                    enet_host_broadcast(server_, 0, outgoing_message.to_enet_packet());
                }
                break;

                case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
                {
                    std::cout << "[Server] Client has timed-out.\n";
                    auto entity_id = reset_player_peer(event.peer);
                    if (entity_id >= 0)
                    {
                        entities_[entity_id].common.active = false;
                    }
                    ToClientNetworkMessage outgoing_message{ToClientMessage::PlayerLeave};
                    enet_host_broadcast(server_, 0, outgoing_message.to_enet_packet());
                }
                break;

                default:
                    break;
            }
        }

        constexpr static float ENTITY_MAX_SPEED = 150.0f;
        for (auto& entity : entities_ | std::ranges::views::drop(MAX_CLIENTS))
        {
            auto& entity_transform = entity.common.transform;
            auto& player_position = entities_[0].common.transform.position;

            auto diff = player_position - entity_transform.position;

            auto len = std::sqrt(diff.x * diff.x + diff.y * diff.y);
            auto move = diff / (len == 0 ? 1 : len);

            auto& velocity = entity_transform.velocity;
            velocity += move * (1.5f + static_cast<float>(entity.common.id) / 100.0f);

            velocity.x = std::clamp(velocity.x, -ENTITY_MAX_SPEED, ENTITY_MAX_SPEED) * 0.90f;
            velocity.y = std::clamp(velocity.y, -ENTITY_MAX_SPEED, ENTITY_MAX_SPEED) * 0.90f;
            entity_transform.position += velocity;
        }

        ToClientNetworkMessage snapshot(ToClientMessage::Snapshot);
        snapshot.payload << entities_.size();
        for (const auto& entity : entities_)
        {
            snapshot.payload << entity.common.id << entity.last_processed
                             << entity.common.transform.position.x
                             << entity.common.transform.position.y << entity.common.active;
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
