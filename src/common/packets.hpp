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

struct FileInfo
{
    char     file_name[32];
    uint16_t file_size;
    uint8_t  hash[16];
};

struct FilesStartPacket
{
    PacketHeader header;
    uint8_t      file_count;
    FileInfo     file_info;
};
