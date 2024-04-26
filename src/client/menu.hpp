#include "packets.hpp"
#include "protocol.hpp"
#include "packet_controller.hpp"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <utility>
class Menu
{
    enum Command : uint8_t
    {
        GET_SERVER_STATUS
    };

public:

    //Applied what the user wants to the send buffer and returns it's size for the
    //send command
    static std::pair<uint16_t, PacketType> parse_command(const char* input, uint8_t* buffer)
    {
        if(strcmp(input, "status") == 0)
        {
            PacketController::create_server_status_packet(buffer);
            return std::make_pair(4, PacketType::RESP_SERVER_STATUS_RESPONSE);
        }
    }

    static void handle_response_packet(uint8_t* buffer)
    {
        const ServerStatusPacket& resp
            = *reinterpret_cast<ServerStatusPacket*>(buffer);

        switch(resp.header.command)
        {
            case PacketType::RESP_SERVER_STATUS_RESPONSE:
            {
                printf("packet_size: %d cpu:%d mem:%f uptime:%ld\n", resp.header.total_size
                    , resp.cpu_usage, resp.memory_info.total_memory, resp.uptime_seconds);
                break;
            }
            default:
            {
                fprintf(stderr, "UNHANDLED RESPONSE PACKET\n");
                break;
            }
        }
    }
};
