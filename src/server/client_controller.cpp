#include "client_controller.hpp"
#include "constants.hpp"
#include "packets.hpp"
#include "packets_utils.hpp"
#include "protocol.hpp"
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
#include "io.hpp"
#include "compression_controller.hpp"
#include <filesystem>

std::function<void(std::shared_ptr<ConnectionBuffer>)>
ClientController::handlers[UINT8_MAX + 1];

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
            if(cb->connection->is_admin)
            {
                std::cout << "An admin disconnected!\n";
                g_state.b_admin_connected.exchange(false);
            }
            cb->connection->id.clear();
            send_sample_resp(cb, tls_buffer, PacketType::RESP_OK);
        };

    handlers[(int)PacketType::REQ_CURRENT_DIRECTORY]
        = [](std::shared_ptr<ConnectionBuffer> cb)
        {
            create_current_dir_packet(cb->connection->current_dir, tls_buffer);
            cb->connection->send_response_sync(tls_buffer
                , sizeof(PacketCurrentDirectory));
        };

    handlers[(int)PacketType::REQ_REMOVE_PATH]
        = [](std::shared_ptr<ConnectionBuffer> cb)
        {
            auto in_packet = *reinterpret_cast<PacketRemovePath*>(cb->buffer);
            if(!is_valid_filename(in_packet.path))
            {
                send_sample_resp(cb, tls_buffer, PacketType::RESP_IO_ERROR);
                return;
            }
            auto path = cb->connection->current_dir + '/' + in_packet.path;
            if(remove_file(path.c_str()) || remove_directory(path.c_str()))
            {
                send_sample_resp(cb, tls_buffer, PacketType::RESP_OK);
                return;
            }
            send_sample_resp(cb, tls_buffer, PacketType::RESP_IO_ERROR);
        };

    handlers[(int)PacketType::REQ_MKDIR]
        = [](std::shared_ptr<ConnectionBuffer> cb)
        {
            auto in_packet = *reinterpret_cast<PacketMakeDirectory*>(cb->buffer);
            if(!is_valid_filename(in_packet.path))
            {
                send_sample_resp(cb, tls_buffer, PacketType::RESP_IO_ERROR);
                return;
            }
            auto path = cb->connection->current_dir + '/' + in_packet.path;

            if(mkdir(path.c_str(), 0777) != 0)
            {
                send_sample_resp(cb, tls_buffer, PacketType::RESP_IO_ERROR);
                return;
            }

            send_sample_resp(cb, tls_buffer, PacketType::RESP_OK);
        };

    handlers[(int)PacketType::REQ_CHANGE_WORKING_DIRECTORY]
        = [](std::shared_ptr<ConnectionBuffer> cb)
        {
            auto packet = *reinterpret_cast<PacketChangeCurrentDirectory*>(cb->buffer);

            std::string new_dir = cd_command(cb->connection->current_dir, packet.path);
            if(new_dir.empty())
            {
                send_sample_resp(cb, tls_buffer, PacketType::RESP_IO_ERROR);
                return;
            }
            cb->connection->current_dir = new_dir;
            send_sample_resp(cb, tls_buffer, PacketType::RESP_OK);
        };

    handlers[(int)PacketType::REQ_FILE_OPEN]
        = [](std::shared_ptr<ConnectionBuffer> cb)
        {
            auto in_packet = *reinterpret_cast<PacketDownloadFile*>(cb->buffer);

            if(!is_valid_filename(in_packet.path))
            {
                send_sample_resp(cb, tls_buffer, PacketType::RESP_IO_ERROR);
                return;
            }

            char file_path[512];
            sprintf(file_path, "%s/%s", cb->connection->current_dir.c_str(), in_packet.path);
            int file = open(file_path, O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
            if(file <= 0)
            {
                send_sample_resp(cb, tls_buffer, PacketType::RESP_IO_ERROR);
                return;
            }
            struct stat file_stat;
            if(fstat(file, &file_stat) != 0)
            {
                send_sample_resp(cb, tls_buffer, PacketType::RESP_IO_ERROR);
                return;
            }
            off_t    file_size      = file_stat.st_size;
            uint16_t max_chunk_size = UINT16_MAX - sizeof(PacketHeader);
            size_t   chunk_count    = (file_size / max_chunk_size) + (file_size % max_chunk_size != 0);


            cb->connection->download_fd = file;

            create_chunk_info_packet(tls_buffer, chunk_count, in_packet.path);
            cb->connection->send_response_sync(tls_buffer
                , sizeof(PacketDownloadedFileInfo));
        };

    handlers[(int)PacketType::REQ_FILE_DOWNLOAD_CHUNK]
        = [](std::shared_ptr<ConnectionBuffer> cb)
        {
            auto    in_packet = *reinterpret_cast<PacketDownloadChunk*>(cb->buffer);
            int64_t output    = create_chunk_packet(tls_buffer, in_packet.chunk_index, cb->connection->download_fd);
            if(output < 0)
            {
                cb->connection->download_fd = 0;
                send_sample_resp(cb, tls_buffer, PacketType::RESP_IO_ERROR);
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
                send_sample_resp(cb, tls_buffer, PacketType::RESP_IO_ERROR);
                return;
            }
            send_sample_resp(cb, tls_buffer, PacketType::RESP_OK);
        };

    handlers[(int)PacketType::REQ_MOVE_FILE]
        = [](std::shared_ptr<ConnectionBuffer> cb)
        {
            auto in_packet = *reinterpret_cast<PacketMoveFile*>(cb->buffer);

            auto user_dir = "./tmp/" + cb->connection->id;

            fs::path user_dir_path    = fs::weakly_canonical(user_dir);
            fs::path current_dir_path = fs::weakly_canonical(cb->connection->current_dir);

            fs::path source      = fs::weakly_canonical(current_dir_path / in_packet.first_path);
            fs::path destination = fs::weakly_canonical(current_dir_path / in_packet.second_path);

            if(!is_path_within_user_dir(source, user_dir_path) || !is_path_within_user_dir(destination, user_dir_path))
            {
                send_sample_resp(cb, tls_buffer, PacketType::RESP_IO_ERROR);
                return;
            }

            if(!move_file(source, destination))
            {
                send_sample_resp(cb, tls_buffer, PacketType::RESP_IO_ERROR);
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
            auto in_packet = *reinterpret_cast<PacketTransferFileStart*>(cb->buffer);

            if(!is_valid_filename(in_packet.file_name))
            {
                send_sample_resp(cb, tls_buffer, PacketType::RESP_IO_ERROR);
                return;
            }

            char file_path[512];
            snprintf(file_path, sizeof(file_path), "%s/%s", cb->connection->current_dir.c_str(), in_packet.file_name);
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
            create_file_list_packet(cb->connection->current_dir, tls_buffer);
            cb->connection->send_response_sync(tls_buffer, sizeof(PacketFileList));
        };

    handlers[(int)PacketType::REQ_LOGIN]
        = [](std::shared_ptr<ConnectionBuffer> cb)
        {
            DatabaseHandler db;
            auto&           in_packet = *reinterpret_cast<PacketLogin*>(cb->buffer);
            if(db.login(in_packet.username, in_packet.password))
            {
                cb->connection->id          = db.getID(in_packet.username);
                cb->connection->current_dir = std::string(SERVER_FILES_DIR) + '/' + cb->connection->id;
                mkdir(cb->connection->current_dir.c_str(), 0777);

                if(db.isAdmin(in_packet.username))
                {
                    if(g_state.b_admin_connected.load())
                    {
                        std::cout << "An admin is already connected!\n";
                        cb->connection->id.clear();
                        cb->connection->current_dir.clear();
                        send_sample_resp(cb, tls_buffer, PacketType::RESP_ADMIN_ALREADY_CONNECTED);
                        return;
                    }
                    std::cout << "An admin connected!\n";
                    g_state.b_admin_connected.exchange(false);
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

    handlers[(int)PacketType::REQ_COMPRESS] = [](std::shared_ptr<ConnectionBuffer> cb)
        {
            auto                     in_packet    = *reinterpret_cast<PacketCompress*>(cb->buffer);
            std::string              archive_path = cb->connection->current_dir + '/' + in_packet.archive_name;
            std::vector<std::string> files_to_compress;

            if(in_packet.compress_all)
            {
                for(const auto& entry : std::filesystem::recursive_directory_iterator(cb->connection->current_dir + '/'))
                {
                    if(entry.is_regular_file())
                    {
                        files_to_compress.push_back(entry.path().string());
                    }
                }
            }
            else
            {
                for(int i = 0; i < in_packet.file_count; ++i)
                {
                    std::string file_path = cb->connection->current_dir + '/' + in_packet.file_names[i];
                    if(!does_file_exist(file_path.c_str()))
                    {
                        send_sample_resp(cb, tls_buffer, PacketType::RESP_IO_ERROR);
                        return;
                    }
                    files_to_compress.push_back(file_path);
                }
            }

            int  compression_level = get_compression_level(in_packet.compression_level, in_packet.format);
            bool success           = false;

            switch(in_packet.format)
            {
                case Format::ZSTD:
                {
                    success = compress_with_zstd(files_to_compress, archive_path, compression_level);
                    break;
                }
                case Format::GZIP:
                {
                    success = compress_with_gzip(files_to_compress, archive_path, compression_level);
                    break;
                }
                case Format::XZ:
                case Format::LZMA:
                {
                    success = compress_with_xz(files_to_compress, archive_path, compression_level);
                    break;
                }
                case Format::LZ4:
                {
                    success = compress_with_lz4(files_to_compress, archive_path, compression_level);
                    break;
                }
                case Format::ZIP:
                {
                    success = compress_with_zip(files_to_compress, archive_path, compression_level);
                    break;
                }
            }

            if(success)
            {
                send_sample_resp(cb, tls_buffer, PacketType::RESP_COMPRESS);
            }
            else
            {
                send_sample_resp(cb, tls_buffer, PacketType::RESP_IO_ERROR);
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
