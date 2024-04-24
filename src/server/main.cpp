#include <atomic>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>
#include <stack>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <netinet/in.h>

#include "client_controller.hpp"
#include "protocol.hpp"
#include "thread_pool.hpp"

#define PORT 1312
#define BACKLOG UINT16_MAX

struct ServerState {
  using TPendingConnections =
      std::stack<std::shared_ptr<ConnectionWrapper>,
                 std::vector<std::shared_ptr<ConnectionWrapper>>>;

  TPendingConnections pending_connections;
  std::mutex pending_connections_mtx;

  std::vector<std::shared_ptr<ConnectionWrapper>> active_connections;
  std::stack<std::shared_ptr<ConnectionBuffer>> completed_packets;
  ThreadPool thread_pool;

  std::atomic_bool b_should_run = true;
  std::atomic_int server_socket_fd = -1;
};

static ServerState g_state;

void make_socket_non_blocking(int socket_fd) {
  int flags = fcntl(socket_fd, F_GETFL, 0);
  if (fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
    // Handle error sometime
  }
}

void handle_packets() {
  while (!g_state.completed_packets.empty()) {
    auto &p = g_state.completed_packets.top();

    g_state.thread_pool.dispatch_async([_p = std::move(p)]() {
      ClientController::process_packet(std::move(_p));
    });

    g_state.completed_packets.pop();
  }
}

void accept_thread_handler() {
  int server_socket_fd;
  if ((server_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }
  int tmp;
  setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(int));
  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);
  int address_length = sizeof(address);
  if (bind(server_socket_fd, (struct sockaddr *)&address, address_length) < 0) {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  if (listen(server_socket_fd, BACKLOG) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }
  g_state.server_socket_fd.exchange(server_socket_fd);

  while (true) {
    int client_socket_fd = accept(server_socket_fd, (struct sockaddr *)&address,
                                  (socklen_t *)&address_length);
    if (client_socket_fd == -1) {
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

void recieve_all_connections() {
  for (auto it = g_state.active_connections.begin();
       it != g_state.active_connections.end(); ++it) {
    auto &connection = *it;

    ssize_t bytes_read =
        recv(connection->socket_fd, connection->buffer + connection->buffer_pos,
             sizeof(connection->buffer) - connection->buffer_pos, 0);

    if (bytes_read == -1) {
      if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
        continue;
      }
      perror("lost connection");
      it = g_state.active_connections.erase(it);
      if (it == g_state.active_connections.end()) {
        break;
      }
    }

    connection->buffer_pos += bytes_read;

    if (connection->buffer_pos < 4) {
      continue;
    }

    auto packet_size =
        *reinterpret_cast<const uint16_t *>(connection->buffer + 2);

    while (true) {
      if (connection->buffer_pos >= packet_size) {
        auto packet = std::make_shared<ConnectionBuffer>();
        packet->connection = connection.get();
        memcpy(packet->buffer, connection->buffer, packet_size);
        g_state.completed_packets.push(std::move(packet));

        connection->buffer_pos = 0;
      }

      auto delta = connection->buffer_pos - packet_size;

      if (delta == 0) {
        connection->buffer_pos = 0;
        break;
      }

      memmove(connection->buffer, connection->buffer + connection->buffer_pos,
              delta);
      connection->buffer_pos = delta;

      packet_size = *reinterpret_cast<const uint16_t *>(connection->buffer + 2);

      if (delta < packet_size) {
        break;
      }
    }
  }
}

void handle_pending_connections() {
  std::lock_guard guard{g_state.pending_connections_mtx};

  while (!g_state.pending_connections.empty()) {
    auto &conn = g_state.pending_connections.top();

    g_state.active_connections.emplace_back(std::move(conn));

    g_state.pending_connections.pop();
  }
}

void server_thread() {
  for (; g_state.b_should_run.load(std::memory_order::relaxed);) {
    // Needs to be ran on a sepparate thread because of while(true)
    recieve_all_connections();
    handle_packets();
    handle_pending_connections();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

void TerminationRequestHandler(int32_t signal) {
  if (g_state.b_should_run.exchange(false)) {
    const auto s = g_state.server_socket_fd.exchange(-1);
    if (s > 0) {
      close(s);
      exit(EXIT_SUCCESS);
    }
  }
}

int main() {
  // TODO: Fix ctrl + c
  (void)::signal(SIGINT,
                 &TerminationRequestHandler); // Interrupt signal (CTRL + C)
  (void)::signal(SIGTERM, &TerminationRequestHandler); // Termination request
  (void)::signal(SIGKILL, &TerminationRequestHandler);

  ClientController::initialize();

  g_state.thread_pool.start(6);
  auto accept_thread = std::jthread(accept_thread_handler);

  server_thread();
}
