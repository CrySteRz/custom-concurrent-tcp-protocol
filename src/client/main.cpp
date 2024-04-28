#include "menu.hpp"
#include "packet_controller.hpp"
#include "packets.hpp"
#include "protocol.hpp"
#include "utils.hpp"
#include <arpa/inet.h>
#include <cstdint>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_PORT 1312
#define SERVER_IP   "127.0.0.1"
#define BUFFER_SIZE 2048

int main()
{
    int                sock;
    struct sockaddr_in server_addr;
    uint8_t            receive_buffer[BUFFER_SIZE] = {0};
    uint8_t            send_buffer[BUFFER_SIZE]    = {0};
    char               input[BUFFER_SIZE];
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == -1)
    {
        perror("socket");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    if(connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr))
        == -1)
    {
        perror("connect");
        return EXIT_FAILURE;
    }

    bool is_authenticated = false;
    char username[32];
    char password[32];
    printf("Enter username: ");
    scanf("%s", username);
    printf("Enter password: ");
    scanf("%s", password);
    uint16_t send_buffer_size = PacketController::create_authenticate_packet(send_buffer, username, password);
    if (send(sock, send_buffer, send_buffer_size, 0) == -1) {
        perror("send");
        return EXIT_FAILURE;
    }
    recv_from_server(sock, receive_buffer, sizeof(receive_buffer));
    const PacketHeader& header = *reinterpret_cast<PacketHeader*>(receive_buffer);
    if (header.command == PacketType::RESP_AUTHENTICATE) {
       const AuthenticateResponsePacket& resp = *reinterpret_cast<AuthenticateResponsePacket*>(receive_buffer);
        if (resp.status){
            is_authenticated = true;
            std::cout << "Authentication successful." << std::endl;
        } else {
            std::cerr << "Authentication failed." << std::endl;
            return EXIT_FAILURE;
        }

    } else {
        std::cerr << "Authentication failed." << std::endl;
        return EXIT_FAILURE;
    }
    
    while (1) {
        try {
            if (!is_authenticated) {
                std::cerr << "Not authenticated." << std::endl;
            }else {
                printf(">>> ");
            scanf("%s", input);
            uint16_t send_buffer_size;
            PacketType response_packet_type;
            auto resp = Menu::parse_command(input, send_buffer);
            if (send(sock, send_buffer, send_buffer_size, 0) == -1) {
                perror("send");
                break;
            }
            recv_from_server(sock, receive_buffer, sizeof(receive_buffer));
            Menu::handle_response_packet(receive_buffer);
            }
        } catch (const std::runtime_error& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    close(sock);
    return EXIT_SUCCESS;
}