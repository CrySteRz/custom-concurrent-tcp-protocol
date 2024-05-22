#include "packets.hpp"
#include "protocol.hpp"
#include "packet_controller.hpp"
#include "utils.hpp"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <optional>
#include <sys/stat.h>
#include <unistd.h>

void print_settings(const GetSettingsPacket& p)
{
    printf("Compression Level: %d\n", p.compression_level);
}

void print_connections_info(const PacketConnectionsInfo& p)
{

    printf("Packets waiting to be processed: %ld\tActive connection count: %ld\tPending connection count: %ld\n"
        , p.completed_packets, p.active_connection_count, p.pending_connection_count);
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

#define BUFFER_SIZE UINT16_MAX
void handle_file_opened_resp(PacketOpenedFileInfo p, int sock)
{
    uint8_t receive_buffer[BUFFER_SIZE] = {0};
    int     file                        = open(p.file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

    //Send a request for each chunk and append the response bytes to the file
    for(size_t chunk = 0; chunk != p.chunks; chunk++)
    {
        PacketDownloadChunk p;
        p.header.command    = PacketType::REQ_FILE_DOWNLOAD_CHUNK;
        p.chunk_index       = chunk;
        p.header.total_size = sizeof(PacketDownloadChunk);
        if(send(sock, &p, p.header.total_size, 0) == -1)
        {
            perror("send");
            break;
        }
        recv_from_server(sock, receive_buffer, sizeof(receive_buffer));
        const PacketTransferFileChunk resp
            = *reinterpret_cast<PacketTransferFileChunk*>(receive_buffer);

        write(file, resp.file_data, resp.header.total_size - sizeof(PacketHeader));
    }

    close(file);
    printf("Downloaded file\n");
}
void parse_mv_command(const char* command, char** path1, char** path2)
{
    char* cmd_copy = strdup(command);

    char* token = strtok(cmd_copy, " ");

    token  = strtok(NULL, " ");
    *path1 = strdup(token);

    token  = strtok(NULL, " ");
    *path2 = strdup(token);

    free(cmd_copy);
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

        if(strncmp(input, "cwd", 3) == 0)
        {
            auto packet = PacketController::create_get_cwd_packet();

            return std::vector<Packet>{packet};
        }

        if(strncmp(input, "open", 4) == 0)
        {
            auto packet = PacketController::create_open_file_packet(input + 5);

            return std::vector<Packet>{packet};
        }

        if(strncmp(input, "rm", 2) == 0)
        {
            auto packet = PacketController::create_remove_path_packet(input + 3);

            return std::vector<Packet>{packet};
        }

        if(strncmp(input, "mkdir", 5) == 0)
        {
            auto packet = PacketController::create_mkdir_packet(input + 6);

            return std::vector<Packet>{packet};
        }

        if(strncmp(input, "cd", 2) == 0)
        {
            auto packet = PacketController::create_change_wd_packet(input + 3);

            return std::vector<Packet>{packet};
        }

        if(strncmp(input, "mv", 2) == 0)
        {
            char* path1, * path2;
            parse_mv_command(input, &path1, &path2);
            auto packet = PacketController::create_mv_packet(path1, path2);

            return std::vector<Packet>{packet};
        }

        if(strncmp(input, "getid", 5) == 0)
        {
            auto packet = PacketController::create_get_id_packet();

            return std::vector<Packet>{packet};
        }

        if(strncmp(input, "logout", 6) == 0)
        {
            auto packet = PacketController::create_logout_packet();

            return std::vector<Packet>{packet};
        }

        if(strncmp(input, "connections", 11) == 0)
        {
            auto packet = PacketController::create_connections_status_packet();

            return std::vector<Packet>{packet};
        }

        if(strncmp(input, "ping", 4) == 0)
        {
            auto packet = PacketController::create_ping_packet();

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
        printf("There are %d files in the current directory!\n", p.file_count);

        for(size_t i = 0; i < p.file_count; i++)
        {
            puts(p.file_names[i]);
        }

        if(p.file_count != 0)
        {
            puts("\n");
        }
    }

    static bool handle_response_packet(uint8_t* buffer, int sock)
    {
        const Packet& r
            = *reinterpret_cast<Packet*>(buffer);
        printf("Packet: %d\n", r.header.command);

        switch(r.header.command)
        {
            case PacketType::RESP_CURRENT_USER:
            {
                const PacketCurrentUser& resp
                    = *reinterpret_cast<PacketCurrentUser*>(buffer);
                printf("Current user id:%s\n", resp.id);
                break;
            }
            case PacketType::RESP_CURRENT_DIRECTORY:
            {
                const PacketCurrentDirectory& resp
                    = *reinterpret_cast<PacketCurrentDirectory*>(buffer);
                printf("Current directory: %s\n", resp.path);
                break;
            }
            case PacketType::RESP_FILE_OPENED:
            {
                const PacketOpenedFileInfo& resp
                    = *reinterpret_cast<PacketOpenedFileInfo*>(buffer);
                printf("Handling downloading file\n");
                handle_file_opened_resp(resp, sock);
                break;
            }
            case PacketType::RESP_CONNECTIONS_INFO:
            {
                const PacketConnectionsInfo& resp
                    = *reinterpret_cast<PacketConnectionsInfo*>(buffer);
                print_connections_info(resp);
                break;
            }
            case PacketType::RESP_PONG:
            {
                printf("PONG!\n");
                break;
            }
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

            case PacketType::RESP_IO_ERROR:
            {
                printf("IO Error\n");
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
