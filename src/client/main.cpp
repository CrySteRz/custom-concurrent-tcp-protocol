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

    while(1)
    {
        printf(">>> ");
        scanf("%2047[^\n]%*c", input);
        auto resp = Menu::parse_command_to_packets(input);

        if(!resp.has_value())
        {
            printf("%s isn't a valid command\n", input);
            continue;
        }
        std::cout << resp.value().size() << std::endl;

        for(auto& packet : resp.value())
        {
            if(send(sock, &packet, packet.header.total_size, 0) == -1)
            {
                perror("send");
                break;
            }


            recv_from_server(sock, receive_buffer, sizeof(receive_buffer));
            Menu::handle_response_packet(receive_buffer);
        }
    }

    close(sock);

    return EXIT_SUCCESS;
}
