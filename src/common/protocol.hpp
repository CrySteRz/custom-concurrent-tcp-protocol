#pragma once

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <sys/socket.h>
#include <thread>

enum class PacketType : uint8_t // Ensure this is one byte
{
  REQ_AUTHENTICATE,
  REQ_FILE_TRANSFER_START,
  REQ_FILE_TRANSFER_CHUNK,
  REQ_FILE_TRANSFER_END,
  REQ_CRC_VERIFY,
  REQ_SERVER_STATUS,
  REQ_GET_SETTINGS,
  RESP_AUTHENTICATE,
  RESP_OK,
  RESP_ERROR,
  RESP_GET_SETTINGS,
  RESP_SERVER_STATUS_RESPONSE,
  RESP_FILE_TRANSFER_START,
  RESP_FILE_TRANSFER_CHUNK,
  RESP_FILE_TRANSFER_END,
  RESP_CRC_FAILED,
  RESP_CRC_OK,

};

struct PacketHeader {
  uint8_t version;
  PacketType command;
  uint16_t total_size;
};

struct Packet {
  PacketHeader header;
  uint8_t buffer[UINT16_MAX - sizeof(PacketHeader)];
};

struct ConnectionWrapper {
  uint32_t socket_fd;
  uint8_t buffer[UINT16_MAX];
  uint16_t buffer_pos;
  bool is_admin;

  void send_response_sync(uint8_t *buffer, size_t length) {
    int sent_bytes = 0;
    while (true) {
      const int result =
          send(this->socket_fd, buffer + sent_bytes, length - sent_bytes, 0);
      if (result == -1) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
          continue;
        }
        perror("send");
        return;
      }
      sent_bytes += result;
      if (sent_bytes >= length) {
        return;
      }
    }
  }
};

struct ConnectionBuffer {
  PacketType get_packet_type() { return (PacketType)connection->buffer[1]; }
  ConnectionWrapper *connection;
  uint8_t buffer[UINT16_MAX];
};
