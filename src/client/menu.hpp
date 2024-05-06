#include "packets.hpp"
#include "protocol.hpp"
#include "packet_controller.hpp"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <optional>
#include <utility>

inline std::vector<const char*> extract_file_paths(const char* input)
{
    std::vector<const char*> file_paths;
    const char*              delimiter = " ";
    const char*              token     = std::strtok(const_cast<char*>(input), delimiter);

    while(token != nullptr)
    {
        file_paths.push_back(token);
        token = std::strtok(nullptr, delimiter);
    }

    return file_paths;
}

class Menu
{
    enum Command : uint8_t
    {
        GET_SERVER_STATUS
    };

public:

    //Applied what the user wants to the send buffer and returns it's size for the
    //send command
    static std::optional<std::vector<Packet>> parse_command_to_packets(char* input)
    {
        if(strncmp(input, "status", 6) == 0)
        {
            auto packet = PacketController::create_server_status_packet();

            return std::vector<Packet>{packet};
        }

        if(strncmp(input, "send", 4) == 0)
        {
            auto file_paths = extract_file_paths(input + 4);

            std::vector<Packet> packets;

            for(auto path : file_paths)
            {
                auto file_packets = PacketController::create_file_transfer_packets(path, UINT16_MAX - sizeof(PacketHeader));
                packets.insert(packets.end(), file_packets.begin(), file_packets.end());
            }

            return packets;
        }

        return {};
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
