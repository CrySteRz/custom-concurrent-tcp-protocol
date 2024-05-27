#pragma once

#include "connection.hpp"
#include "thread_pool.hpp"
#include <memory>
#include <stack>
#include <vector>
struct ServerState
{
    using TPendingConnections
        = std::stack<std::shared_ptr<ConnectionWrapper>
            , std::vector<std::shared_ptr<ConnectionWrapper>>>;

    TPendingConnections pending_connections;
    std::mutex          pending_connections_mtx;

    std::vector<std::shared_ptr<ConnectionWrapper>> active_connections;
    std::stack<std::shared_ptr<ConnectionBuffer>>   completed_packets;
    ThreadPool                                      thread_pool;

    std::atomic_bool b_should_run      = true;
    std::atomic_bool b_admin_connected = false;
    std::atomic_int  server_socket_fd  = -1;

    //We only lock on writes since all our settings are primitives and we cannot have dataraces on reads
    //If we will have something more complex such as a string we will use the ReadWrite lock we write instead
    //Also technically the mutex doesn't really help much
    std::mutex settings_write_lock;
    uint8_t    compression_level = 1;
};

static ServerState g_state;
