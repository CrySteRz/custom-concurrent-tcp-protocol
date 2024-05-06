#pragma once
#include "../server/server_info.hpp"
#include "protocol.hpp"

struct ServerStatusPacket
{
    PacketHeader header;
    MemoryInfo   memory_info;
    int8_t       cpu_usage;
    long         uptime_seconds;
};

struct SamplePacket
{
    PacketHeader header;
};

struct PacketTransferFileStart
{
    PacketHeader header;
    uint8_t      hash[16];
    char         file_name[255];
};

struct PacketTransferFileChunk
{
    PacketHeader header;
    uint8_t      file_data[UINT16_MAX - sizeof(PacketHeader)];
};

struct PacketTransferFileEnd
{
    PacketHeader header;
    uint8_t      file_data[UINT16_MAX - sizeof(PacketHeader)];
};

struct PacketLogin
{
    PacketHeader header;
    char         username[64];
    char         password[64];
};
