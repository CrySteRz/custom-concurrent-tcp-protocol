

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <sys/socket.h>
#include <sys/types.h>
#include <cstring> 
#include <algorithm>

inline bool recv_from_server(int socket_fd, uint8_t *buffer, size_t length) {
    // Read the packet header first
    int bytes_read = recv(socket_fd, buffer, sizeof(PacketHeader), MSG_WAITALL); // Use MSG_WAITALL to ensure the header is fully received
    if (bytes_read == -1) {
        perror("recv error");
        return false;
    } else if (bytes_read < sizeof(PacketHeader)) {
        fprintf(stderr, "Incomplete header: only %d bytes read\n", bytes_read);
        return false;
    }

    // Interpret the packet header to get the total size
    const PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
    if (header->total_size > UINT16_MAX || header->total_size < sizeof(PacketHeader)) {
        fprintf(stderr, "Invalid packet size: %u bytes\n", header->total_size);
        return false;
    }

    // Read the remaining part of the packet
    size_t remaining_size = header->total_size - bytes_read;
    while (remaining_size > 0) {
        size_t to_read = std::min(remaining_size, length - bytes_read);
        int part_read = recv(socket_fd, buffer + bytes_read, to_read, 0);
        if (part_read == -1) {
            perror("recv error during body read");
            return false;
        } else if (part_read == 0) {
            fprintf(stderr, "Connection closed by peer\n");
            return false;
        }
        bytes_read += part_read;
        remaining_size -= part_read;
    }

    return true;
}