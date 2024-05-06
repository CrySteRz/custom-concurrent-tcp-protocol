#include "client_controller.hpp"
#include "packets.hpp"
#include "protocol.hpp"
#include "server_info.hpp"
#include <cstdint>
#include <cstdio>
#include <fcntl.h>
#include <memory>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

std::function<void(std::shared_ptr<ConnectionBuffer>)>
ClientController::handlers[UINT8_MAX + 1];

void send_resp_continue(std::shared_ptr<ConnectionBuffer> cb, uint8_t* buffer, PacketType packet_type)
{
    auto& p = *reinterpret_cast<SamplePacket*>(buffer);
    p.header.command    = packet_type;
    p.header.total_size = sizeof(SamplePacket);
    cb->connection->send_response_sync(buffer
        , sizeof(SamplePacket));
}


void ClientController::initialize()
{
    set_handler<PacketType::REQ_SERVER_STATUS>(
        [](std::shared_ptr<ConnectionBuffer> cb)
    {
        auto& p             = *reinterpret_cast<ServerStatusPacket*>(tls_buffer);
        p.header.command    = PacketType::RESP_SERVER_STATUS_RESPONSE;
        p.header.total_size = sizeof(ServerStatusPacket);
        p.cpu_usage         = ServerInfo::get_cpu_usage_percentage();
        p.memory_info       = ServerInfo::get_memory_usage();
        p.uptime_seconds    = ServerInfo::get_system_uptime_seconds();

        cb->connection->send_response_sync(tls_buffer
        , sizeof(ServerStatusPacket));
    });

    handlers[(int)PacketType::REQ_CRC_VERIFY]
        = [](std::shared_ptr<ConnectionBuffer> cb)
        {};

    handlers[(int)PacketType::REQ_FILE_TRANSFER_START]
        = [](std::shared_ptr<ConnectionBuffer> cb)
        {
            char working_dir[255], file_path[255];
            sprintf(working_dir, "./tmp/%d", cb->connection->id);
            auto packet = *reinterpret_cast<PacketTransferFileStart*>(cb->buffer);
            sprintf(file_path, "%s/%s", working_dir, packet.file_name);
            mkdir(working_dir, 0700);

            int file = open(file_path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

            cb->connection->fd = file;

            send_resp_continue(cb, tls_buffer, PacketType::RESP_CONTINUE);
        };

    handlers[(int)PacketType::REQ_FILE_TRANSFER_CHUNK]
        = [](std::shared_ptr<ConnectionBuffer> cb)
        {
            auto& in_packet = *reinterpret_cast<PacketTransferFileChunk*>(cb->buffer);

            write(cb->connection->fd, in_packet.file_data, in_packet.header.total_size - sizeof(PacketHeader));

            send_resp_continue(cb, tls_buffer, PacketType::RESP_CONTINUE);
        };

    handlers[(int)PacketType::REQ_FILE_TRANSFER_END]
        = [](std::shared_ptr<ConnectionBuffer> cb)
        {
            auto& in_packet = *reinterpret_cast<PacketTransferFileEnd*>(cb->buffer);

            write(cb->connection->fd, in_packet.file_data, in_packet.header.total_size - sizeof(PacketHeader));

            send_resp_continue(cb, tls_buffer, PacketType::RESP_FILE_TRANSFER_OK);
        };

    handlers[(int)PacketType::REQ_GET_SETTINGS]
        = [](std::shared_ptr<ConnectionBuffer> cb)
        {};

    handlers[(int)PacketType::REQ_SET_SETTING]
        = [](std::shared_ptr<ConnectionBuffer> cb)
        {};
}

void ClientController::process_packet(std::shared_ptr<ConnectionBuffer> cb)
{
    auto packet_type = cb->get_packet_type();

    handlers[(int)packet_type](cb);
}

thread_local uint8_t
ClientController::tls_buffer[std::numeric_limits<uint16_t>::max()]{};
