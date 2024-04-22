#include "protocol.hpp"
#include "thread_pool.hpp"
#include "client_controller.hpp"
#include <memory>
#include <stack>
#include <vector>

struct ServerState
{
    std::vector<std::unique_ptr<ConnectionWrapper>> pending_connections;
    std::mutex                                      pending_connections_mtx;

    std::vector<std::unique_ptr<ConnectionWrapper>> active_connections;
    std::stack<ConnectionBuffer>                    completed_packets;
    ThreadPool                                      thread_pool;
};

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
//void accept_thread_handler() {
//while (true) {
//int client_socket_fd = accept(server_socket_fd, ...);
//if (client_socket_fd == -1) {
//break;
//}
//auto cw = std::make_unique<ConnectionWrapper>(socket_client_fd);
//start_processing_client(std::move(cw));
//}
//}

//O sa fie un thread care doar accepta conexiuni, de aia avem nevoie de mutex, cand o conexiune e acceptata
//se baga pe active_connections.
//cand un packet e completed e mutat pe stack-ul de completed_packets
//recieving thread o sa ruleze pe vectorul de active_connections (async) si cand e cazul sa fie unul gata
//se pune in completed_packets


int main()
{
    ClientController::initialize();
}
