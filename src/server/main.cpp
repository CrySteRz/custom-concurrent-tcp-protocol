#include "protocol.hpp"
#include <memory>
#include <stack>
#include <vector>

void handle_packets()
{
    //Pseudocode atm, needs the thread_pool implementation
    //for(auto p : completed_packets)
    //{
    //thread_pool.dispatch_async([p]()
    //{
    //ClientController::process_packet(p);
    //})
    //}
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
struct ServerState
{
    std::vector<std::unique_ptr<ConnectionWrapper>> pending_connections;
    std::mutex                                      pending_connections_mtx;

    std::vector<std::unique_ptr<ConnectionWrapper>> active_connections;

    std::stack<ConnectionBuffer> completed_packets;
};

int main()
{}
