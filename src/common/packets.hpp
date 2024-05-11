#pragma once
#include "../server/server_info.hpp"
#include "protocol.hpp"
#include <cstdio>

#define MAX_FILES        128
#define MAX_FILENAME_LEN 256

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
    char         file_name[MAX_FILENAME_LEN];
};

enum class Format : uint8_t
{
    ZTSD, GZIP, XZ, LZMA, LZ4
};

struct PacketArchiveFiles
{
    PacketHeader   header;
    Format         format;
    uint8_t        compression_level;
    unsigned short file_count;
    bool           compress_all;
    //Max 128 files ig
    char file_names[MAX_FILES][MAX_FILENAME_LEN];
};

struct PacketFileList
{
    PacketHeader header;
    uint8_t      file_count;
    char         file_names[MAX_FILES][MAX_FILENAME_LEN];
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
