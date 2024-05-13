#include "packets.hpp"
#include "protocol.hpp"
#include "packet_controller.hpp"
#include "utils.hpp"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <optional>

void print_settings(GetSettingsPacket p)
{
    printf("Compression Level: %d\n", p.compression_level);
}

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

        if(strncmp(input, "get_settings", 12) == 0)
        {
            auto packet = PacketController::create_get_settings_packet();

            return std::vector<Packet>{packet};
        }

        if(strncmp(input, "set", 3) == 0)
        {
            auto packet = PacketController::create_set_setting_packet(input + 3);

            if(packet.has_value())
            {
                return std::vector<Packet>{packet.value()};
            }
        }

        if(strncmp(input, "login", 5) == 0)
        {
            char* username;
            char* password;

            username = strtok((char*)input + 6, " ");
            password = strtok(NULL, " ");

            return std::vector<Packet>{PacketController::create_login_packet(username, password)};
        }

        if(strncmp(input, "send", 4) == 0)
        {
            auto file_paths = extract_file_paths(input + 4);

            std::vector<Packet> packets;

            for(auto path : file_paths)
            {
                auto file_packets = create_file_transfer_packets(path, UINT16_MAX - sizeof(PacketHeader));
                packets.insert(packets.end(), file_packets.begin(), file_packets.end());
            }

            return packets;
        }

        if(strncmp(input, "get", 3) == 0)
        {
            std::vector<Packet> packets;
        }

        if(strncmp(input, "ls", 2) == 0)
        {
            return std::vector<Packet>{PacketController::create_ls_packet()};
        }

        return {};
    }

    static void list_files(const PacketFileList& p)
    {
        printf("There are %d files uploaded!\n", p.file_count);

        for(size_t i = 0; i < p.file_count; i++)
        {
            puts(p.file_names[i]);
        }

        if(p.file_count != 0)
        {
            puts("\n");
        }
    }

    static bool handle_response_packet(uint8_t* buffer)
    {
        const Packet& r
            = *reinterpret_cast<Packet*>(buffer);
        printf("Packet: %d\n", r.header.command);

        switch(r.header.command)
        {
            case PacketType::RESP_FILE_LIST:
            {
                const PacketFileList& resp
                    = *reinterpret_cast<PacketFileList*>(buffer);
                list_files(resp);
                break;
            }
            case PacketType::RESP_SERVER_STATUS_RESPONSE:
            {
                const ServerStatusPacket& resp
                    = *reinterpret_cast<ServerStatusPacket*>(buffer);
                printf("packet_size: %d cpu:%d mem:%f uptime:%ld\n", resp.header.total_size
                    , resp.cpu_usage, resp.memory_info.total_memory, resp.uptime_seconds);
                break;
            }
            case PacketType::RESP_SETTINGS:
            {
                const GetSettingsPacket& resp
                    = *reinterpret_cast<GetSettingsPacket*>(buffer);
                print_settings(resp);
                break;
            }
            case PacketType::RESP_CONTINUE:
            {
                break;
            }
            case PacketType::RESP_FILE_TRANSFER_OK:
            {
                printf("File transfer was ok!\n");
            }
            break;

            case PacketType::RESP_OK:
            {
                printf("Command was succesful\n");
            }
            break;

            case PacketType::RESP_REQUIRES_ADMIN:
            {
                printf("The server requires admin for that command, please login using the login command\n");
                return true;
            }

            case PacketType::RESP_NOT_LOGGED_IN:
            {
                printf("You are not logged in\n");
                return true;
            }

            case PacketType::RESP_BAD_LOGIN:
            {
                printf("Bad username/password\n");
                return true;
            }

            default:
            {
                fprintf(stderr, "UNHANDLED RESPONSE PACKET\n");
            }
            break;

        }

        return false;
    }
};
