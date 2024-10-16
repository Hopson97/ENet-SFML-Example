#pragma once

#include <SFML/Network/Packet.hpp>
#include <enet/enet.h>

#include "Common.h"

enum class ToServerMessageType : u8
{
    None,
    Message,

    Input,
};

enum class ToClientMessage : u8
{
    None,

    ClientInfo,

    Message,
    PlayerJoin,
    PlayerLeave,

    Snapshot,
};

template <typename E>
concept NetworkMessageType =
    std::is_same_v<E, ToServerMessageType> || std::is_same_v<E, ToClientMessage>;

template <NetworkMessageType MessageType>
struct NetworkMessage
{
    NetworkMessage(MessageType message_type) noexcept
    {
        uint16_t message = static_cast<uint16_t>(message_type);
        payload << message;
    }

    NetworkMessage(ENetPacket* enet_packet) noexcept
    {
        if (enet_packet)
        {
            payload.append(enet_packet->data, enet_packet->dataLength);
            uint16_t message;
            payload >> message;
            message_type = static_cast<MessageType>(message);
        }
    }

    ENetPacket* to_enet_packet(ENetPacketFlag flags = ENET_PACKET_FLAG_RELIABLE) const noexcept
    {
        return enet_packet_create(payload.getData(), payload.getDataSize(), flags);
    }

    sf::Packet payload;
    MessageType message_type = MessageType::None;
};

using ToServerNetworkMessage = NetworkMessage<ToServerMessageType>;
using ToClientNetworkMessage = NetworkMessage<ToClientMessage>;