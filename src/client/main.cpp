#include "memory.h"
#include "packets.hpp"
#include "protocol.hpp"
#include "utils.hpp"
#include <arpa/inet.h>
#include <cstdint>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_PORT 1312
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 2048

int main() {
  int sock;
  struct sockaddr_in server_addr;
  uint8_t receive_buffer[BUFFER_SIZE] = {0};
  uint8_t send_buffer[BUFFER_SIZE] = {0};
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) {
    perror("socket");
    return 1;
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(SERVER_PORT);
  inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

  if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==
      -1) {
    perror("connect");
    return EXIT_FAILURE;
  }

  // comunicarea cu server-ul
  while (1) {
    printf(">>> ");
    scanf("%s", send_buffer);
    Packet packet;
    packet.header.command = PacketType::REQ_SERVER_STATUS;
    packet.header.version = 0;
    packet.header.total_size = 4;
    memcpy(send_buffer, &packet.header, sizeof(PacketHeader));
    printf("packetsize %d", *(uint16_t *)(send_buffer + 2));

    if (send(sock, send_buffer, 8, 0) == -1) {
      perror("send");
      break;
    }

    // Instead of overwriting the entire buffer we only null terminate after the
    // read bytes
    recv_from_server(sock, receive_buffer, sizeof(receive_buffer));
    // receive_buffer[read_bytes] = 0;
    const ServerStatusPacket &resp =
        *reinterpret_cast<ServerStatusPacket *>(receive_buffer);

    printf("packet_size: %d cpu:%d mem:%f uptime:%ld", resp.header.total_size,
           resp.cpu_usage, resp.memory_info.total_memory, resp.uptime_seconds);
  }

  close(sock);

  return EXIT_SUCCESS;
}