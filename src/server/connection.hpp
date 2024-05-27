#pragma once

#include "protocol.hpp"
#include <cstddef>
#include <cstdint>
#include <string>

struct ConnectionWrapper
{
    uint32_t    socket_fd;
    uint8_t     buffer[UINT16_MAX];
    uint16_t    buffer_pos;
    std::string current_dir;
    bool        is_admin;
    int         fd          = 0;
    int         download_fd = 0;
    std::string id;

    ~ConnectionWrapper();

    void send_response_sync(uint8_t* buffer, size_t length);
};

struct ConnectionBuffer
{
    PacketType get_packet_type();
    ConnectionWrapper* connection;
    uint8_t            buffer[UINT16_MAX];
};
