#pragma once

#include "packets.hpp"
#include "protocol.hpp"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <memory>
#include <sys/types.h>

inline std::vector<uint8_t> read_entire_file(const char* path)
{
    std::ifstream file(path, std::ios::binary);
    if(!file.is_open())
    {
        throw;
    }

    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0);

    std::vector<uint8_t> data(size);
    file.read(reinterpret_cast<char*>(data.data()), size);
    file.close();

    return data;
}

inline std::vector<std::vector<uint8_t>> break_vector_into(uint16_t size, std::vector<uint8_t> data)
{
    std::vector<std::vector<uint8_t>> result;

    size_t dataSize  = data.size();
    size_t num_vecs  = dataSize / size;
    size_t remaining = dataSize % size;

    for(size_t i = 0; i < num_vecs; ++i)
    {
        result.push_back(std::vector<uint8_t>(data.begin() + i * size, data.begin() + (i + 1) * size));
    }

    if(remaining > 0)
    {
        result.push_back(std::vector<uint8_t>(data.begin() + num_vecs * size, data.end()));
    }

    return result;
}

class PacketController
{
public:
    static Packet create_server_status_packet()
    {
        Packet packet;
        packet.header.command    = PacketType::REQ_SERVER_STATUS;
        packet.header.version    = 0;
        packet.header.total_size = 4;

        return packet;
    }

    static std::vector<Packet> create_file_transfer_packets(const char* file_path, uint16_t chunk_size)
    {
        auto file_content = read_entire_file(file_path);

        auto file_chunks = break_vector_into(chunk_size, file_content);

        std::vector<Packet> packets;

        PacketTransferFileStart p;
        strcpy(p.file_name, file_path);
        p.header.command    = PacketType::REQ_FILE_TRANSFER_START;
        p.header.total_size = sizeof(PacketTransferFileStart);

        packets.push_back(*reinterpret_cast<Packet*>(&p));

        for(auto chunk : file_chunks)
        {
            Packet p;
            p.header.total_size = sizeof(PacketHeader) + chunk.size();

            p.header.command = PacketType::REQ_FILE_TRANSFER_CHUNK;
            memcpy(p.buffer, chunk.data(), chunk.size());
            packets.push_back(p);
        }

        packets.back().header.command = PacketType::REQ_FILE_TRANSFER_END;

        return packets;
    }
};
