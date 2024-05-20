#include "client_controller.hpp"
#include "packets.hpp"
#include "protocol.hpp"
#include "server_info.hpp"
#include "sqlite3.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <mutex>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include "db_handler.hpp"
#include "state.hpp"


std::function<void(std::shared_ptr<ConnectionBuffer>)>
ClientController::handlers[UINT8_MAX + 1];

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

void write_user_file_list(char* users_dir, uint8_t* buffer)
{
    auto& p   = *reinterpret_cast<PacketFileList*>(buffer);
    DIR*  dir = opendir(users_dir); //User has no files or dir
    if(dir == nullptr)
    {
        p.file_count = 0;
        return;
    }

    struct dirent* entry;
    size_t         count = 0;

    while((entry = readdir(dir)) != nullptr && count < MAX_FILES)
    {
        if((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0))
        {
            continue;
        }

        //Copy the file name into the packet's file_names array
        strncpy(p.file_names[count], entry->d_name, MAX_FILENAME_LEN);
        strncpy(p.file_names[count], entry->d_name, MAX_FILENAME_LEN);
        p.file_names[count][MAX_FILENAME_LEN - 1] = '\0'; //Ensure null termination

        ++count;
    }

    p.file_count = static_cast<uint8_t>(count);

    closedir(dir);
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

void create_current_user_packet(std::string& id, uint8_t* buffer)
{
    auto& p = *reinterpret_cast<PacketCurrentUser*>(buffer);
    p.header.total_size = sizeof(PacketCurrentUser);
    p.header.command    = PacketType::RESP_CURRENT_USER;
    strncpy(p.id, id.c_str(), sizeof(p.id));
}

void create_file_list_packet(std::string& uid, uint8_t* buffer)
{
    char users_dir[255];
    sprintf(users_dir, "./tmp/%s", uid.c_str());
    write_user_file_list(users_dir, buffer);
    auto& p = *reinterpret_cast<PacketFileList*>(buffer);
    p.header.command    = PacketType::RESP_FILE_LIST;
    p.header.total_size = sizeof(PacketFileList);
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

void ClientController::initialize()
{
    set_handler<PacketType::REQ_SERVER_STATUS>(
        [](std::shared_ptr<ConnectionBuffer> cb)
    {
        if(!cb->connection->is_admin)
        {
            send_resp_not_admin(cb, tls_buffer);
            return;
        }

        create_server_status_packet(tls_buffer);

        cb->connection->send_response_sync(tls_buffer
        , sizeof(ServerStatusPacket));
    });


    handlers[(int)PacketType::REQ_CONNECTIONS_INFO]
        = [](std::shared_ptr<ConnectionBuffer> cb)
        {
            if(!cb->connection->is_admin)
            {
                send_resp_not_admin(cb, tls_buffer);
                return;
            }

            create_connections_info_packet(tls_buffer);

            cb->connection->send_response_sync(tls_buffer
                , sizeof(PacketConnectionsInfo));
        };

    handlers[(int)PacketType::REQ_CRC_VERIFY]
        = [](std::shared_ptr<ConnectionBuffer> cb)
        {};

    handlers[(int)PacketType::REQ_LOGOUT]
        = [](std::shared_ptr<ConnectionBuffer> cb)
        {
            cb->connection->id.clear();
            send_sample_resp(cb, tls_buffer, PacketType::RESP_OK);
        };

    handlers[(int)PacketType::REQ_FILE_OPEN]
        = [](std::shared_ptr<ConnectionBuffer> cb)
        {
            auto in_packet = *reinterpret_cast<PacketOpenFile*>(cb->buffer);

            char file_path[512];
            sprintf(file_path, "./tmp/%s/%s", cb->connection->id.c_str(), in_packet.path);
            int file = open(file_path, O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
            if(file <= 0)
            {
                send_sample_resp(cb, tls_buffer, PacketType::RESP_NO_SUCH_FILE);
                return;
            }
            struct stat file_stat;
            if(fstat(file, &file_stat) != 0)
            {
                send_sample_resp(cb, tls_buffer, PacketType::RESP_NO_SUCH_FILE);
                return;
            }
            off_t    file_size      = file_stat.st_size;
            uint16_t max_chunk_size = UINT16_MAX - sizeof(PacketHeader);
            size_t   chunk_count    = (file_size / max_chunk_size) + (file_size % max_chunk_size != 0);


            cb->connection->download_fd = file;

            create_chunk_info_packet(tls_buffer, chunk_count, in_packet.path);
            cb->connection->send_response_sync(tls_buffer
                , sizeof(PacketOpenedFileInfo));
        };

    handlers[(int)PacketType::REQ_FILE_DOWNLOAD_CHUNK]
        = [](std::shared_ptr<ConnectionBuffer> cb)
        {
            auto    in_packet = *reinterpret_cast<PacketDownloadChunk*>(cb->buffer);
            int64_t output    = create_chunk_packet(tls_buffer, in_packet.chunk_index, cb->connection->download_fd);
            if(output < 0)
            {
                cb->connection->download_fd = 0;
                send_sample_resp(cb, tls_buffer, PacketType::RESP_NO_SUCH_FILE);
            }
            auto out_packet = *reinterpret_cast<PacketTransferFileChunk*>(tls_buffer);

            cb->connection->send_response_sync(tls_buffer
                , out_packet.header.total_size);
        };

    handlers[(int)PacketType::REQ_FILE_CLOSE]
        = [](std::shared_ptr<ConnectionBuffer> cb)
        {
            if(close(cb->connection->download_fd) < 0)
            {
                close(cb->connection->download_fd);
                cb->connection->download_fd = 0;
                send_sample_resp(cb, tls_buffer, PacketType::RESP_NO_SUCH_FILE);
                return;
            }
            send_sample_resp(cb, tls_buffer, PacketType::RESP_OK);
        };

    handlers[(int)PacketType::REQ_PING]
        = [](std::shared_ptr<ConnectionBuffer> cb)
        {
            send_sample_resp(cb, tls_buffer, PacketType::RESP_PONG);
        };

    handlers[(int)PacketType::REQ_GET_CURRENT_USER]
        = [](std::shared_ptr<ConnectionBuffer> cb)
        {
            create_current_user_packet(cb->connection->id, tls_buffer);
            cb->connection->send_response_sync(tls_buffer
                , sizeof(PacketCurrentUser));
        };

    handlers[(int)PacketType::REQ_FILE_TRANSFER_START]
        = [](std::shared_ptr<ConnectionBuffer> cb)
        {
            char working_dir[255], file_path[512];
            snprintf(working_dir, sizeof(working_dir), "./tmp/%s", cb->connection->id.c_str());
            auto packet = *reinterpret_cast<PacketTransferFileStart*>(cb->buffer);
            snprintf(file_path, sizeof(file_path), "%s/%s", working_dir, packet.file_name);
            mkdir(working_dir, 0700);
            int file = open(file_path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
            cb->connection->fd = file;
            send_sample_resp(cb, tls_buffer, PacketType::RESP_CONTINUE);
        };

    handlers[(int)PacketType::REQ_FILE_TRANSFER_CHUNK]
        = [](std::shared_ptr<ConnectionBuffer> cb)
        {
            auto& in_packet = *reinterpret_cast<PacketTransferFileChunk*>(cb->buffer);

            write(cb->connection->fd, in_packet.file_data, in_packet.header.total_size - sizeof(PacketHeader));

            send_sample_resp(cb, tls_buffer, PacketType::RESP_CONTINUE);
        };

    handlers[(int)PacketType::REQ_FILE_TRANSFER_END]
        = [](std::shared_ptr<ConnectionBuffer> cb)
        {
            auto& in_packet = *reinterpret_cast<PacketTransferFileEnd*>(cb->buffer);

            write(cb->connection->fd, in_packet.file_data, in_packet.header.total_size - sizeof(PacketHeader));

            send_sample_resp(cb, tls_buffer, PacketType::RESP_FILE_TRANSFER_OK);
        };

    handlers[(int)PacketType::REQ_GET_SETTINGS]
        = [](std::shared_ptr<ConnectionBuffer> cb)
        {
            if(!cb->connection->is_admin)
            {
                send_resp_not_admin(cb, tls_buffer);
                return;
            }
            create_settings_resp_packet(tls_buffer);
            cb->connection->send_response_sync(tls_buffer, sizeof(PacketFileList));
        };

    handlers[(int)PacketType::REQ_SET_SETTING]
        = [](std::shared_ptr<ConnectionBuffer> cb)
        {
            if(!cb->connection->is_admin)
            {
                send_resp_not_admin(cb, tls_buffer);
                return;
            }
            std::lock_guard<std::mutex> guard(g_state.settings_write_lock);
            SetSettingPacket            p = *reinterpret_cast<SetSettingPacket*>(&cb->connection->buffer);
            parse_seting_packet_and_set(p);
            send_sample_resp(cb, tls_buffer, PacketType::RESP_OK);
        };

    handlers[(int)PacketType::REQ_FILE_LIST]
        = [](std::shared_ptr<ConnectionBuffer> cb)
        {
            create_file_list_packet(cb->connection->id, tls_buffer);
            cb->connection->send_response_sync(tls_buffer, sizeof(PacketFileList));
        };

    handlers[(int)PacketType::REQ_LOGIN]
        = [](std::shared_ptr<ConnectionBuffer> cb)
        {
            DatabaseHandler db;
            auto&           in_packet = *reinterpret_cast<PacketLogin*>(cb->buffer);
            if(db.login(in_packet.username, in_packet.password))
            {
                cb->connection->id = db.getID(in_packet.username);
                if(db.isAdmin(in_packet.username))
                {
                    cb->connection->is_admin = true;
                }
                send_sample_resp(cb, tls_buffer, PacketType::RESP_OK);
            }
            else
            {
                send_sample_resp(cb, tls_buffer, PacketType::RESP_BAD_LOGIN);
                return;
            }
        };
}

void ClientController::process_packet(std::shared_ptr<ConnectionBuffer> cb)
{
    auto packet_type = cb->get_packet_type();
    if((packet_type != PacketType::REQ_LOGIN) && (packet_type != PacketType::REQ_PING) && (packet_type != PacketType::REQ_LOGOUT)
        && cb->connection->id.empty())
    {
        send_resp_unauthorized(cb, tls_buffer);
        return;
    }

    handlers[(int)packet_type](cb);
}

thread_local uint8_t
ClientController::tls_buffer[std::numeric_limits<uint16_t>::max()]{};
