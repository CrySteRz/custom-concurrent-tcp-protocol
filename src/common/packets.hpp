#pragma once
#include "../server/server_info.hpp"
#include "protocol.hpp"
#include <cstdint>
#include <cstdio>

#define MAX_PATH_LENGTH 512

struct ServerStatusPacket
{
    PacketHeader header;
    MemoryInfo   memory_info;
    int8_t       cpu_usage;
    long         uptime_seconds;
};

enum class Setting : uint8_t
{
    COMPRESSION_LEVEL
};

struct SetSettingPacket
{
    PacketHeader header;
    Setting      setting;
    uint8_t      value[8];
};

struct GetSettingsPacket
{
    PacketHeader header;
    uint8_t      compression_level;
};
struct LoggedInPacket
{
    PacketHeader header;
    bool         is_admin;
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

//Client connection statuses
struct PacketConnectionsInfo
{
    PacketHeader header;
    size_t       active_connection_count;
    size_t       pending_connection_count;
    size_t       completed_packets;
};

enum class Format : uint8_t
{
    ZSTD, GZIP, XZ, LZMA, LZ4, ZIP
};
enum class Level : uint8_t
{
    FASTEST, FAST, NORMAL, GOOD, BEST
};

struct PacketCompress
{
    PacketHeader   header;
    Format         format;
    Level          compression_level;
    bool           compress_all;
    uint8_t        file_count;
    char file_names[MAX_FILES][MAX_FILENAME_LEN];
    char archive_name[MAX_FILENAME_LEN];
};

struct PacketFileList
{
    PacketHeader header;
    uint8_t      file_count;
    char         file_names[MAX_FILES][MAX_FILENAME_LEN];
};

struct PacketCurrentUser
{
    PacketHeader header;
    char         id[255];
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


struct PacketDownloadFile
{
    PacketHeader header;
    char         path[255];
};

struct PacketDownloadedFileInfo
{
    PacketHeader header;
    uint64_t     chunks;
    char         file_name[255];
};

struct PacketDownloadChunk
{
    PacketHeader header;
    uint64_t     chunk_index;
};

struct PacketCurrentDirectory
{
    PacketHeader header;
    char         path[512];
};

typedef PacketCurrentDirectory PacketChangeCurrentDirectory;

typedef PacketCurrentDirectory PacketRemovePath;

typedef PacketCurrentDirectory PacketMakeDirectory;

struct PacketMoveFile
{
    PacketHeader header;
    char         first_path[512];
    char         second_path[512];
};
struct PacketCopyFile
{
    PacketHeader header;
    char         first_path[512];
    char         second_path[512];
};







