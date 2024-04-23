#include "protocol.hpp"
#include <cstddef>
#include <cstring>
#include <fcntl.h>
#include "thread_pool.hpp"
#include "cstring"
#include "client_controller.hpp"
#include <cstdint>
#include <memory>
#include <netinet/in.h>

#include <stack>
#include <vector>

#define PORT    1312
#define BACKLOG UINT16_MAX

struct ServerState
{
    std::vector<std::unique_ptr<ConnectionWrapper>> pending_connections;
    std::mutex                                      pending_connections_mtx;

    std::vector<std::unique_ptr<ConnectionWrapper>> active_connections;
    std::stack<ConnectionBuffer>                    completed_packets;
    ThreadPool                                      thread_pool;
};

void make_socket_non_blocking(int socket_fd)
{
    int flags = fcntl(socket_fd, F_GETFL, 0);
    if(fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        //Handle error sometime
    }
}

void handle_packets(ServerState& state)
{
    while(!state.completed_packets.empty())
    {
        auto p = std::make_shared<ConnectionBuffer>(state.completed_packets.top());

        state.thread_pool.dispatch_async([p]()
        {
            ClientController::process_packet(p);
        });

        state.completed_packets.pop();
    }
}

//Pseudocode atm, needs other stuff
void accept_thread_handler(ServerState& state)
{
    int server_socket_fd;
    if((server_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in address;
    address.sin_family      = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port        = htons(PORT);
    int address_length = sizeof(address);
    if(bind(server_socket_fd, (struct sockaddr*)&address, address_length) < 0)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if(listen(server_socket_fd, BACKLOG) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while(true)
    {
        int client_socket_fd = accept(server_socket_fd, (struct sockaddr*)&address, (socklen_t*)&address_length);
        if(client_socket_fd == -1)
        {
            break;
        }
        make_socket_non_blocking(client_socket_fd);
        auto cw = std::make_unique<ConnectionWrapper>(client_socket_fd);
        state.active_connections.emplace_back(std::move(cw));
    }
}

void recieve_all_connections(ServerState& state)
{
    for(size_t idx = 0; idx != state.active_connections.size(); idx++)
    {
        auto&   connection = state.active_connections[idx];
        ssize_t bytes_read = recv(connection->socket_fd, connection->buffer + connection->buffer_pos
                , sizeof(connection->buffer), 0);
        connection->buffer_pos += bytes_read;
        if(bytes_read < 0)
        {
            state.active_connections.erase(state.active_connections.begin() + idx);
            continue;
        }
        //EOF
        if(bytes_read == 0)
        {
            ConnectionBuffer packet;
            packet.connection = connection.get();
            memcpy(packet.buffer, connection->buffer, sizeof(packet.buffer));
            //Shouldn't be needed but whatever
            memset(connection->buffer, 0, sizeof(connection->buffer));
            state.completed_packets.push(packet);
            continue;
        }
    }
}

//O sa fie un thread care doar accepta conexiuni, de aia avem nevoie de mutex, cand o conexiune e acceptata
//se baga pe active_connections.
//cand un packet e completed e mutat pe stack-ul de completed_packets
//recieving thread o sa ruleze pe vectorul de active_connections (async) si cand e cazul sa fie unul gata
//se pune in completed_packets

void server_thread(ServerState& state)
{
    for(;;)
    {
        //Needs to be ran on a sepparate thread because of while(true)
        accept_thread_handler(state);

        recieve_all_connections(state);
        handle_packets(state);
        //handle_pending_connections(state);
    }
}

int main()
{
    ClientController::initialize();

}
