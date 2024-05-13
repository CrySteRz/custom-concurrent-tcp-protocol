#include <atomic>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>
#include "state.hpp"
#include <fcntl.h>
#include <netinet/in.h>

#include "client_controller.hpp"
#include "protocol.hpp"

#define PORT    1312
#define BACKLOG UINT16_MAX

void make_socket_non_blocking(int socket_fd)
{
    int flags = fcntl(socket_fd, F_GETFL, 0);
    if(fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        perror("Cannot make nonblocking");
    }
}

void handle_packets()
{
    while(!g_state.completed_packets.empty())
    {
        auto& p = g_state.completed_packets.top();

        g_state.thread_pool.dispatch_async([_p = std::move(p)]()
        {
            ClientController::process_packet(std::move(_p));
        });

        g_state.completed_packets.pop();
    }
}

void accept_thread_handler()
{
    int server_socket_fd;
    if((server_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    int tmp;
    setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(int));
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
    g_state.server_socket_fd.exchange(server_socket_fd);

    while(true)
    {
        int client_socket_fd = accept(server_socket_fd, (struct sockaddr*)&address
                , (socklen_t*)&address_length);
        if(client_socket_fd == -1)
        {
            break;
        }
        make_socket_non_blocking(client_socket_fd);
        auto cw = std::make_shared<ConnectionWrapper>(client_socket_fd);
        {
            std::lock_guard guard{g_state.pending_connections_mtx};
            g_state.pending_connections.push(std::move(cw));
        }
    }
}

void receive_all_connections()
{
    auto it = g_state.active_connections.begin();

    while(it != g_state.active_connections.end())
    {
        auto& connection = *it;

        ssize_t bytes_read = recv(connection->socket_fd, connection->buffer + connection->buffer_pos
                , sizeof(connection->buffer) - connection->buffer_pos, 0);

        if(bytes_read == -1)
        {
            if((errno == EAGAIN) || (errno == EWOULDBLOCK))
            {
                ++it; //Only increment here if no erasing occurs
                continue;
            }
            perror("lost connection");
            close(connection->socket_fd);
            it = g_state.active_connections.erase(it); //Update iterator here
            if(it == g_state.active_connections.end())
            {
                break; //Check if the end is reached
            }
            continue;
        }
        else if(bytes_read == 0)
        {
            close(connection->socket_fd);
            it = g_state.active_connections.erase(it);
            if(it == g_state.active_connections.end())
            {
                break;
            }
            continue;
        }

        connection->buffer_pos += bytes_read;

        if(connection->buffer_pos < 4)
        {
            ++it; //Only increment here if no erasing occurs
            continue;
        }

        uint16_t packet_size = *reinterpret_cast<
            const uint16_t*>(connection->buffer + 2);

        while(connection->buffer_pos >= packet_size)
        {
            if(packet_size == 0)
            {
                break; //Avoids infinite loops on zero packet size

            }
            auto packet = std::make_shared<ConnectionBuffer>();
            packet->connection = connection.get();
            memcpy(packet->buffer, connection->buffer, packet_size);
            g_state.completed_packets.push(std::move(packet));
            int delta = connection->buffer_pos - packet_size;

            if(delta == 0)
            {
                connection->buffer_pos = 0;
                break;
            }

            memmove(connection->buffer, connection->buffer + packet_size, delta);
            connection->buffer_pos = delta;

            if(connection->buffer_pos < 4)
            {
                break;
            }

            packet_size = *reinterpret_cast<
                const uint16_t*>(connection->buffer + 2);
        }

        if(it != g_state.active_connections.end())
        {
            ++it; //Only increment here if no erasing occurs
        }

    }
}

void handle_pending_connections()
{
    std::lock_guard guard{g_state.pending_connections_mtx};

    while(!g_state.pending_connections.empty())
    {
        auto& conn = g_state.pending_connections.top();

        g_state.active_connections.emplace_back(std::move(conn));

        g_state.pending_connections.pop();
    }
}

void server_thread()
{
    for(; g_state.b_should_run.load(std::memory_order_relaxed);)
    {
        //Needs to be ran on a sepparate thread because of while(true)
        receive_all_connections();
        handle_packets();
        handle_pending_connections();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void TerminationRequestHandler(int32_t signal)
{
    if(g_state.b_should_run.exchange(false))
    {
        const auto s = g_state.server_socket_fd.exchange(-1);
        if(s > 0)
        {
            close(s);
            exit(EXIT_SUCCESS);
        }
    }
}

int main()
{
    //TODO: Fix ctrl + c
    (void)::signal(SIGINT
        , &TerminationRequestHandler);                   //Interrupt signal (CTRL + C)
    (void)::signal(SIGTERM, &TerminationRequestHandler); //Termination request
    (void)::signal(SIGKILL, &TerminationRequestHandler);

    mkdir("./tmp", 0777);
    srand((unsigned)time(0));
    ClientController::initialize();

    g_state.thread_pool.start(6);
    auto accept_thread = std::thread(accept_thread_handler);

    server_thread();
}
