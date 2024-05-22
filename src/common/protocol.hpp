#pragma once

#include <dirent.h>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <sys/socket.h>
#include <thread>
#include "constants.hpp"

enum class PacketType : uint8_t
{
    REQ_FILE_TRANSFER_START
    , REQ_FILE_TRANSFER_CHUNK
    , REQ_FILE_TRANSFER_END
    , REQ_CRC_VERIFY
    , REQ_SERVER_STATUS
    , REQ_GET_SETTINGS
    , REQ_SET_SETTING
    , REQ_REMOVE_PATH
    , REQ_MOVE_FILE
    , REQ_LOGIN
    , REQ_FILE_LIST
    , REQ_CONNECTIONS_INFO
    , REQ_PING
    , REQ_LOGOUT
    , REQ_GET_CURRENT_USER
    , REQ_FILE_OPEN
    , REQ_FILE_DOWNLOAD_CHUNK
    , REQ_FILE_CLOSE
    , REQ_CURRENT_DIRECTORY
    , REQ_CHANGE_WORKING_DIRECTORY
    , REQ_MKDIR
    , RESP_CURRENT_DIRECTORY
    , RESP_IO_ERROR
    , RESP_FILE_OPENED
    , RESP_FILE_CHUNK
    , RESP_FILE_CLOSED
    , RESP_CONTINUE
    , RESP_OK
    , RESP_CRC_FAILED
    , RESP_SERVER_STATUS_RESPONSE
    , RESP_FILE_TRANSFER_OK
    , RESP_REQUIRES_ADMIN
    , RESP_NOT_LOGGED_IN
    , RESP_CURRENT_USER
    , RESP_BAD_LOGIN
    , RESP_SETTINGS
    , RESP_CONNECTIONS_INFO
    , RESP_PONG
    , RESP_FILE_LIST
};

struct PacketHeader
{
    uint8_t    version;
    PacketType command;
    uint16_t   total_size;
};

struct Packet
{
    PacketHeader header;
    uint8_t      buffer[UINT16_MAX - sizeof(PacketHeader)];
};

struct ConnectionWrapper
{
    uint32_t    socket_fd;
    uint8_t     buffer[UINT16_MAX];
    uint16_t    buffer_pos;
    std::string current_dir;
    bool        is_admin;
    int         fd          = 0;
    int         download_fd = 0;
    std::string id;

    ~ConnectionWrapper()
    {
        close(fd);
        close(download_fd);
        char dir_path[255];
        sprintf(dir_path, "%s/%s", SERVER_FILES_DIR, id.c_str());
        DIR* dir = opendir(dir_path);
        if(dir)
        {
            //TODO: Reenable after debug
            //std::filesystem::remove_all(dir_path);
        }
    }

    void send_response_sync(uint8_t* buffer, size_t length)
    {
        int sent_bytes = 0;

        while(true)
        {
            const int result
                = send(this->socket_fd, buffer + sent_bytes, length - sent_bytes, 0);
            if(result == -1)
            {
                if((errno == EWOULDBLOCK) || (errno == EAGAIN))
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }
                perror("send");
                return;
            }
            sent_bytes += result;
            if(sent_bytes >= length)
            {
                return;
            }
        }
    }
};

struct ConnectionBuffer
{
    PacketType get_packet_type()
    {
        //TODO: Enable this after everything is done
        //if((connection->buffer[1] < 0) || (connection->buffer[1] > (int)PacketType::RESP_FILE_LIST))
        //{
        //return PacketType::REQ_SET_SETTING;
        //}
        return (PacketType)connection->buffer[1];
    }
    ConnectionWrapper* connection;
    uint8_t            buffer[UINT16_MAX];
};
