#include <cstdint>

enum PacketType : uint8_t //Ensure this is one byte
{
    FILES_TRANSFER_START
    , FILE_TRANSFER_CHUNK
    , FILE_TRANSFER_END
    , CRC_VERIFY
    , SERVER_STATUS
    , GET_SETTINGS
    , SET_SETTING
};

struct Protocol
{
    uint8_t    version;
    PacketType command;
    uint16_t   total_size;
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
    PacketType get_packet_type()
    {
        return (PacketType)connection->buffer[1];
    }
    ConnectionWrapper* connection;
    uint8_t            buffer[UINT16_MAX];
};
