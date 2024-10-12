#include "Server.h"

#include <cmath>
#include <print>
#include <ranges>

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

        auto player = (ServerEntity*)peer->data;
        if (player)
        {
            player->peer = nullptr;
            player->common.active = false;
        }
        peer->data = nullptr;
    }
} // namespace

Server::Server()
{
    for (int i = 0; i < entities_.size(); i++)
    {
        entities_[i].common.id = i;
        entities_[i].common.active = i >= MAX_CLIENTS;

        if (i < MAX_CLIENTS)
        {
            entities_[i].common.transform.size = {24, 48};
        }
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
        std::println("An error occurred while trying to create an ENet server host.");
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
            std::println("[Server] Ticks: {} ({} seconds)", ticks, ticks / 20);
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
                    std::println("[Server] A new client connected.");
                    i16 id = 0;
                    for (int i = 0; i < MAX_CLIENTS; i++)
                    {
                        if (!entities_[i].peer)
                        {
                            id = entities_[i].common.id;
                            entities_[i].peer = event.peer;
                            entities_[i].common.active = true;
                            event.peer->data = (void*)&entities_[i];
                            std::println("[Server] New client slot: {}",
                                         (int)entities_[i].common.id);
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
                            std::println("[Server] Got message from client: ", text);

                            ToClientNetworkMessage outgoing_message{ToClientMessage::Message};
                            outgoing_message.payload << text;
                            enet_host_broadcast(server_, 0, outgoing_message.to_enet_packet());
                            enet_host_flush(server_);
                        }
                        break;

                        case ToServerMessageType::Input:
                        {
                            Input input;
                            auto player = (ServerEntity*)event.peer->data;

                            incoming_message.payload >> player->last_processed >> input.dt >>
                                input.keys;

                            // std::println("Got input {} {} from player {}", input.keys, input.dt,
                            // player->common.id);

                            player->input_buffer.push_back(input);

                            // TODO - rather than process input straight away...
                            // process_input_for_player(player->common.transform, input);
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
                    std::println("[Server] Client has disconnected.");
                    reset_player_peer(event.peer);
                    ToClientNetworkMessage outgoing_message{ToClientMessage::PlayerLeave};
                    enet_host_broadcast(server_, 0, outgoing_message.to_enet_packet());
                }
                break;

                case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
                {
                    std::println("[Server] Client has timed-out.");
                    reset_player_peer(event.peer);
                    ToClientNetworkMessage outgoing_message{ToClientMessage::PlayerLeave};
                    enet_host_broadcast(server_, 0, outgoing_message.to_enet_packet());
                }
                break;

                default:
                    break;
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            auto& player = entities_[i];
            if (!player.common.active)
            {

                continue;
            }
            for (const auto& input : player.input_buffer)
            {
                // prevent dts above 60!
                if (input.dt <= 0.16)
                {
                    process_input_for_player(player.common.transform, input);
                    apply_map_collisions(player.common.transform);
                }
            }

            if (player.input_buffer.empty())
            {
                std::println("No player input to process.");
                apply_map_collisions(player.common.transform);
            }
            player.input_buffer.clear();
        }

        for (auto& entity : entities_ | std::ranges::views::drop(MAX_CLIENTS))
        {
            auto& entity_transform = entity.common.transform;
            auto& player_position = entities_[0].common.transform.position;

            auto diff = player_position - entity_transform.position;

            auto len = std::sqrt(diff.x * diff.x + diff.y * diff.y);
            auto move = diff / (len == 0 ? 1 : len);

            auto& velocity = entity_transform.velocity;
            velocity += move * (2.0f + static_cast<float>(entity.common.id) / 100.0f);

            apply_map_collisions(entity_transform);
        }

        ToClientNetworkMessage snapshot(ToClientMessage::Snapshot);
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
