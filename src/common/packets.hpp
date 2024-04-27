#pragma once
#include "../server/server_info.hpp"
#include "protocol.hpp"

struct ServerStatusPacket {
  PacketHeader header;
  MemoryInfo memory_info;
  int8_t cpu_usage;
  long uptime_seconds;
};
