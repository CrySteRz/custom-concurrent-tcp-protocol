#pragma once
#include "../server/server_info.hpp"
#include "protocol.hpp"

struct ServerStatusPacket {
  PacketHeader header;
  MemoryInfo memory_info;
  int8_t cpu_usage;
  long uptime_seconds;
};

// struct DirectoryCreationPacket {
//   PacketHeader header;
//   uint16_t path_length;
//   char path[UINT16_MAX - sizeof(PacketHeader) - sizeof(uint16_t)];
// };
// If we want to maintain directory structure on the server. for recursive directory creation

struct FileTransferStartPacket {
  PacketHeader header;
  uint16_t file_name_length;
  uint32_t file_size;
};

struct FileTransferChunkPacket {
  PacketHeader header;
  uint16_t chunk_size;
  uint8_t chunk_data[UINT16_MAX - sizeof(PacketHeader) - sizeof(uint16_t)];
};

struct FileTransferEndPacket {
  PacketHeader header;
  uint32_t crc32;
};

struct CrcVerifyPacket {
  PacketHeader header;
  uint32_t crc32;
};
