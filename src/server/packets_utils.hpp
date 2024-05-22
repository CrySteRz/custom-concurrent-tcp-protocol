#pragma once

#include "packets.hpp"
#include "state.hpp"
#include <cstring>
#include <string>
#include "io.hpp"

void create_current_user_packet(std::string& id, uint8_t* buffer)
{
    auto& p = *reinterpret_cast<PacketCurrentUser*>(buffer);
    p.header.total_size = sizeof(PacketCurrentUser);
    p.header.command    = PacketType::RESP_CURRENT_USER;
    strncpy(p.id, id.c_str(), sizeof(p.id));
}

void create_file_list_packet(std::string& current_dir, uint8_t* buffer)
{
    write_user_file_list(current_dir.c_str(), buffer);
    auto& p = *reinterpret_cast<PacketFileList*>(buffer);
    p.header.command    = PacketType::RESP_FILE_LIST;
    p.header.total_size = sizeof(PacketFileList);
}

void parse_seting_packet_and_set(SetSettingPacket p)
{
    switch(p.setting)
    {
        case Setting::COMPRESSION_LEVEL:
        {
            //TODO: Add checks
            g_state.compression_level = p.value[0];
            break;
        }
        default:
        {
            return;
        }
    }
}

void create_server_status_packet(uint8_t* buffer)
{
    auto& p = *reinterpret_cast<ServerStatusPacket*>(buffer);
    p.header.command    = PacketType::RESP_SERVER_STATUS_RESPONSE;
    p.header.total_size = sizeof(ServerStatusPacket);
    p.cpu_usage         = ServerInfo::get_cpu_usage_percentage();
    p.memory_info       = ServerInfo::get_memory_usage();
    p.uptime_seconds    = ServerInfo::get_system_uptime_seconds();
}

void create_chunk_info_packet(uint8_t* buffer, uint64_t chunk_count, char* file_name)
{
    auto& p = *reinterpret_cast<PacketOpenedFileInfo*>(buffer);
    p.header.command    = PacketType::RESP_FILE_OPENED;
    p.header.total_size = sizeof(PacketOpenedFileInfo);
    p.chunks            = chunk_count;
    strcpy(p.file_name, file_name);
}

void create_connections_info_packet(uint8_t* buffer)
{
    auto& p = *reinterpret_cast<PacketConnectionsInfo*>(buffer);
    p.header.command           = PacketType::RESP_CONNECTIONS_INFO;
    p.header.total_size        = sizeof(PacketConnectionsInfo);
    p.completed_packets        = g_state.completed_packets.size();
    p.active_connection_count  = g_state.active_connections.size() + 1;
    p.pending_connection_count = g_state.pending_connections.size();
}

void create_settings_resp_packet(uint8_t* buffer)
{
    auto& p = *reinterpret_cast<GetSettingsPacket*>(buffer);
    p.header.command    = PacketType::RESP_SETTINGS;
    p.compression_level = g_state.compression_level;
    p.header.total_size = sizeof(GetSettingsPacket);
}

void create_current_dir_packet(std::string& cwd, uint8_t* buffer)
{
    auto& p = *reinterpret_cast<PacketCurrentDirectory*>(buffer);
    strncpy(p.path, cwd.c_str(), sizeof(p.path));
    p.header.command    = PacketType::RESP_CURRENT_DIRECTORY;
    p.header.total_size = sizeof(PacketCurrentDirectory);
}

int64_t create_chunk_packet(uint8_t* buffer, size_t chunk_index, int fd)
{
    auto& p = *reinterpret_cast<PacketTransferFileChunk*>(buffer);

    uint16_t max_chunk_size = UINT16_MAX - sizeof(PacketHeader);
    off_t    offset         = chunk_index * max_chunk_size;
    if(lseek(fd, offset, SEEK_SET) != offset)
    {
        perror("Failed to seek to chunk position");
        close(fd);
        return -1;
    }

    size_t bytes_read = read(fd, p.file_data, max_chunk_size);
    if(bytes_read < 0)
    {
        perror("Failed to read chunk from file");
        close(fd);
        return -1;
    }

    p.header.command    = PacketType::RESP_FILE_CHUNK;
    p.header.total_size = bytes_read + sizeof(PacketHeader);

    return 0;
}

void send_sample_resp(std::shared_ptr<ConnectionBuffer> cb, uint8_t* buffer, PacketType packet_type)
{
    auto& p = *reinterpret_cast<SamplePacket*>(buffer);
    p.header.command    = packet_type;
    p.header.total_size = sizeof(SamplePacket);
    cb->connection->send_response_sync(buffer
        , sizeof(SamplePacket));
}

void send_resp_not_admin(std::shared_ptr<ConnectionBuffer> cb, uint8_t* buffer)
{
    send_sample_resp(cb, buffer, PacketType::RESP_REQUIRES_ADMIN);
}

void send_resp_unauthorized(std::shared_ptr<ConnectionBuffer> cb, uint8_t* buffer)
{
    send_sample_resp(cb, buffer, PacketType::RESP_NOT_LOGGED_IN);
}
