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
        packet.header.total_size = sizeof(PacketHeader);
        memcpy(buffer, &packet.header, sizeof(PacketHeader));
        std::cout << "Server status packet created" << std::endl;
        return packet.header.total_size;
    }

    static uint16_t create_authenticate_packet(uint8_t* buffer, const char* username, const char* password)
{
    AuthenticatePacket packet = {};
    packet.header.command = PacketType::REQ_AUTHENTICATE;
    packet.header.version = 0;
    packet.header.total_size = sizeof(AuthenticatePacket);
    strncpy(packet.username, username, sizeof(packet.username) - 1);
    strncpy(packet.password, password, sizeof(packet.password) - 1);
    memcpy(buffer, &packet, sizeof(AuthenticatePacket));
    return packet.header.total_size;
}

    static uint16_t create_transfer_packet(uint8_t* buffer, const char* path)
    {
        Packet packet;
        packet.header.command    = PacketType::REQ_FILE_TRANSFER_START;
        packet.header.version    = 0;
        packet.header.total_size = 4;
        memcpy(buffer, &packet.header, sizeof(PacketHeader));
        return packet.header.total_size;
    }
};
