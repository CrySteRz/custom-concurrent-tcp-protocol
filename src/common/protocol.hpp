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
    , REQ_COMPRESS
    , RESP_CURRENT_DIRECTORY
    , RESP_IO_ERROR
    , RESP_FILE_OPENED
    , RESP_FILE_CHUNK
    , RESP_FILE_CLOSED
    , RESP_CONTINUE
    , RESP_ADMIN_ALREADY_CONNECTED
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
    , RESP_COMPRESS
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
