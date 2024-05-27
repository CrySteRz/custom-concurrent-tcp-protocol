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

void print_help_message()
{
    printf("Available Commands:\n");
    printf("status - Retrieves the current server status including CPU, memory usage, and uptime.\n");
    printf("cwd - Gets the current working directory on the server.\n");
    printf("download <path> - Downloads a file from the specified path on the server.\n");
    printf("rm <path> - Removes a file or directory at the specified path.\n");
    printf("mkdir <path> - Creates a new directory at the specified path.\n");
    printf("compress <archive name> <options> - Compresses files into the specified archive format and level. Options include --level [FASTEST|FAST|NORMAL|GOOD|BEST] and --format [GZIP|XZ|LZMA|LZ4|ZIP].\n");
    printf("cd <path> - Changes the current directory to the specified path.\n");
    printf("mv <source> <destination> - Moves or renames a file or directory from source to destination.\n");
    printf("getid - Retrieves the unique identifier for the current session.\n");
    printf("logout - Logs out from the current session.\n");
    printf("connections - Displays the number of active and pending connections as well as packets waiting to be processed.\n");
    printf("ping - Sends a ping to the server to check connectivity.\n");
    printf("get_settings - Retrieves current settings related to compression and file handling.\n");
    printf("set <setting> - Updates settings based on the provided parameters.\n");
    printf("login <username> <password> - Logs into the server with the specified username and password.\n");
    printf("send <file paths> - Sends one or more files to the server.\n");
    printf("ls - Lists files in the current directory.\n");
    printf("help - Displays this help message.\n");
    printf("\nFor more information on each command, type 'help <command>'.\n");
}


void print_settings(const GetSettingsPacket& p)
{
    printf("Compression Level: %d\n", p.compression_level);
}

std::string format_to_string(Format format)
{
    switch(format)
    {
        case Format::ZSTD:
        {
            return "ZSTD";
        }
        case Format::GZIP:
        {
            return "GZIP";
        }
        case Format::XZ:
        {
            return "XZ";
        }
        case Format::LZMA:
        {
            return "LZMA";
        }
        case Format::LZ4:
        {
            return "LZ4";
        }
        case Format::ZIP:
        {
            return "ZIP";
        }
        default:
        {
            return "UNKNOWN";
        }
    }
}

std::string level_to_string(Level level)
{
    switch(level)
    {
        case Level::FASTEST:
        {
            return "FASTEST";
        }
        case Level::FAST:
        {
            return "FAST";
        }
        case Level::NORMAL:
        {
            return "NORMAL";
        }
        case Level::GOOD:
        {
            return "GOOD";
        }
        case Level::BEST:
        {
            return "BEST";
        }
        default:
        {
            return "UNKNOWN";
        }
    }
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
void handle_file_opened_resp(PacketDownloadedFileInfo p, int sock)
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

        if(strncmp(input, "download", 8) == 0)
        {
            auto packet = PacketController::create_download_file_packet(input + 9);

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

        if(strncmp(input, "compress", 8) == 0)
        {
            return handle_compress_command(input + 9);
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

        if(strncmp(input, "help", 4) == 0)
        {
            print_help_message();
            return {};
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

    static std::optional<std::vector<Packet>> handle_compress_command(const char* input)
    {
        std::istringstream       iss(input);
        std::string              archive_name, level_str, format_str, temp;
        std::vector<std::string> paths;
        Format                   format            = Format::ZSTD;
        Level                    compression_level = Level::NORMAL;
        bool                     compress_all      = false;
        iss >> archive_name;

        while(iss >> temp)
        {
            if(temp == "--level")
            {
                iss >> level_str;
            }
            else if(temp == "--format")
            {
                iss >> format_str;
            }
            else if(temp == "*")
            {
                compress_all = true;
            }
            else
            {
                paths.push_back(temp);
            }
        }

        if(level_str == "FASTEST")
        {
            compression_level = Level::FASTEST;
        }
        else if(level_str == "FAST")
        {
            compression_level = Level::FAST;
        }
        else if(level_str == "NORMAL")
        {
            compression_level = Level::NORMAL;
        }
        else if(level_str == "GOOD")
        {
            compression_level = Level::GOOD;
        }
        else if(level_str == "BEST")
        {
            compression_level = Level::BEST;
        }
        else
        {
            printf("Using %s as default level\n", level_to_string(compression_level).c_str());
        }

        if(format_str == "GZIP")
        {
            format = Format::GZIP;
        }
        else if(format_str == "XZ")
        {
            format = Format::XZ;
        }
        else if(format_str == "LZMA")
        {
            format = Format::LZMA;
        }
        else if(format_str == "LZ4")
        {
            format = Format::LZ4;
        }
        else if(format_str == "ZIP")
        {
            format = Format::ZIP;
        }
        else
        {
            printf("Using %s as default format\n", format_to_string(format).c_str());
        }

        printf("Creating compress packet\n");
        Packet packet = PacketController::create_compress_packet(format, compression_level, compress_all, paths, archive_name.c_str());
        return std::vector<Packet>{packet};
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
        printf("Packet: %d\n", static_cast<int>(r.header.command));

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
                const PacketDownloadedFileInfo& resp
                    = *reinterpret_cast<PacketDownloadedFileInfo*>(buffer);
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
            case PacketType::RESP_ADMIN_ALREADY_CONNECTED:
            {
                printf("An admin is already connected!\n");
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
            case PacketType::RESP_COMPRESS:
            {
                printf("Compress command was successful\n");
                break;
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
