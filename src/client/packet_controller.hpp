#pragma once

#include "protocol.hpp"
#include <cstring>
#include <functional>
#include <memory>
#include <sys/types.h>

class PacketController
{
public:
    static uint16_t create_server_status_packet(uint8_t* buffer)
    {
        Packet packet;
        packet.header.command    = PacketType::REQ_SERVER_STATUS;
        packet.header.version    = 0;
        packet.header.total_size = 4;
        memcpy(buffer, &packet.header, sizeof(PacketHeader));
        return packet.header.total_size;
    }
};
