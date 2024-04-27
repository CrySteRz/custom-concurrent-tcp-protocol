#include "packets.hpp"
#include "protocol.hpp"
#include "packet_controller.hpp"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <utility>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
namespace fs = std::filesystem;
class Menu
{
    enum Command : uint8_t
    {
        GET_SERVER_STATUS,
        TRANSFER
        
    };

public:
    // TODO: Implement the Packet for file transfer
static void transfer_file(const fs::path& file_path, uint8_t* buffer) {
        std::cout << "Transferring file: " << file_path << std::endl;
    }
    //TODO implement the Packet for directory transfer/creation
    static void transfer_directory(const fs::path& dir_path, uint8_t* buffer) {
        std::cout << "Creating directory on server: " << dir_path << std::endl;
        for (const auto& entry : fs::recursive_directory_iterator(dir_path)) {
            if (fs::is_directory(entry)) {
                std::cout << "Creating sub-directory on server: " << entry.path() << std::endl;
            } else {
                transfer_file(entry.path(), buffer);
            }
        }
    }

    static std::pair<uint16_t, PacketType> parse_command(const char* input, uint8_t* buffer) {
        std::string command(input);
        std::istringstream iss(command);
        std::string token;
        iss >> token; // Iau primul cuvant (commands) ca sa putem parsa path-urile apoi
        if (token == "status") {
            PacketController::create_server_status_packet(buffer);
            return std::make_pair(4, PacketType::RESP_SERVER_STATUS_RESPONSE);
        } else if (token == "transfer") {
            std::vector<std::string> paths;
            while (iss >> token) {
                paths.push_back(token);
            }
            if (paths.empty()) {
                throw std::runtime_error("No paths provided.");
            } else {
                for (const auto& path : paths) {
                    fs::path fs_path(path);
                    if (fs::is_directory(fs_path)) {
                        transfer_directory(fs_path, buffer);
                    } else {
                        transfer_file(fs_path, buffer);
                    }
                }
            }
        } else {
            iss.clear();
        }
        return std::make_pair(0, PacketType::RESP_OK);
    }



    static void handle_response_packet(uint8_t* buffer)
{
    const PacketHeader& header = *reinterpret_cast<PacketHeader*>(buffer);

    switch(header.command)
    {
        case PacketType::RESP_SERVER_STATUS_RESPONSE:
        {
            const ServerStatusPacket& resp = *reinterpret_cast<ServerStatusPacket*>(buffer);
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
