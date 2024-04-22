#pragma once

#include <cstdint>

enum ClientRequestPacket : uint8_t //Ensure this is one byte
{
    REQ_FILES_TRANSFER_START
    , REQ_FILE_TRANSFER_CHUNK
    , REQ_FILE_TRANSFER_END
    , REQ_CRC_VERIFY
    , REQ_SERVER_STATUS
    , REQ_GET_SETTINGS
    , REQ_SET_SETTING
};

enum ServerResponse : uint8_t
{
    RESP_OK
    , RESP_CRC_FAILED
    , RESP_SERVER_STATUS_RESPONSE
};

struct ServerProtocol
{
    uint8_t             version;
    ClientRequestPacket req_type;
    uint16_t            total_size;
    uint8_t             buffer[UINT16_MAX - sizeof(uint16_t) - sizeof(uint8_t) - sizeof(ClientRequestPacket)];
};

struct ClientProtocol
{
    uint8_t        version;
    ServerResponse resp_type;
    uint16_t       total_size;
    uint8_t        buffer[UINT16_MAX - sizeof(uint16_t) - sizeof(uint8_t) - sizeof(ClientRequestPacket)];
};

struct ConnectionWrapper
{
    uint32_t socket_fd;
    uint8_t  buffer[UINT16_MAX];
    uint16_t buffer_pos;
    bool     is_admin;
    uint32_t id; //Just in case
};

struct ConnectionBuffer
{
    ClientRequestPacket get_packet_type()
    {
        return (ClientRequestPacket)connection->buffer[1];
    }
    ConnectionWrapper* connection;
    uint8_t            buffer[UINT16_MAX];
};
