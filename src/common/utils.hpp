#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <sys/socket.h>
#include <sys/types.h>

inline bool recv_from_server(uint32_t socket, uint8_t* buffer, size_t length)
{
    int bytes_read = recv(socket, buffer, 4, 0);
    if(bytes_read == -1)
    {
        perror("lost connection");
        return false;
    }

    const auto packet_size = *reinterpret_cast<uint16_t*>(buffer + 2);
    if(packet_size == 4)
    {
        return true;
    }
    size_t recv_count = 4;

    while(recv_count < packet_size)
    {
        int bytes_read
            = recv(socket, buffer + recv_count, packet_size - recv_count, 0);

        if(bytes_read == -1)
        {
            perror("lost connection");
            return false;
        }

        recv_count += bytes_read;
    }

    return true;
}
