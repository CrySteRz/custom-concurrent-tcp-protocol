#include "client_controller.hpp"
#include "packets.hpp"
#include "protocol.hpp"
#include "server_info.hpp"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <sys/socket.h>

std::function<void(std::shared_ptr<ConnectionBuffer>)>
    ClientController::handlers[UINT8_MAX + 1];

void ClientController::initialize() {
  set_handler<PacketType::REQ_SERVER_STATUS>(
      [](std::shared_ptr<ConnectionBuffer> cb) {
        auto &p = *reinterpret_cast<ServerStatusPacket *>(tls_buffer);
        p.header.command = PacketType::RESP_SERVER_STATUS_RESPONSE;
        p.header.total_size = sizeof(ServerStatusPacket);
        p.cpu_usage = ServerInfo::get_cpu_usage_percentage();
        p.memory_info = ServerInfo::get_memory_usage();
        p.uptime_seconds = ServerInfo::get_system_uptime_seconds();
        cb->connection->send_response_sync(tls_buffer,
                                           sizeof(ServerStatusPacket));
      });

  handlers[(int)PacketType::REQ_CRC_VERIFY] =
      [](std::shared_ptr<ConnectionBuffer> cb) {};

  handlers[(int)PacketType::REQ_FILES_TRANSFER_START] =
      [](std::shared_ptr<ConnectionBuffer> cb) {};

  handlers[(int)PacketType::REQ_FILE_TRANSFER_CHUNK] =
      [](std::shared_ptr<ConnectionBuffer> cb) {};

  handlers[(int)PacketType::REQ_FILE_TRANSFER_END] =
      [](std::shared_ptr<ConnectionBuffer> cb) {};

  handlers[(int)PacketType::REQ_GET_SETTINGS] =
      [](std::shared_ptr<ConnectionBuffer> cb) {};

  handlers[(int)PacketType::REQ_SET_SETTING] =
      [](std::shared_ptr<ConnectionBuffer> cb) {};
}

void ClientController::process_packet(std::shared_ptr<ConnectionBuffer> cb) {
  auto packet_type = cb->get_packet_type();
  handlers[(int)packet_type](cb);
}

thread_local uint8_t
    ClientController::tls_buffer[std::numeric_limits<uint16_t>::max()]{};