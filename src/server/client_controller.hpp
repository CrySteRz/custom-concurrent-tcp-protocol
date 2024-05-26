#include "protocol.hpp"
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>

class ConnectionBuffer;

class ClientController {
public:
  template <PacketType P, typename TFunctor>
  static void set_handler(TFunctor &&functor) {
    handlers[(int)P] = std::move(functor);
  }
  static void process_packet(std::shared_ptr<ConnectionBuffer> cb);
  static void initialize();

  static std::function<void(std::shared_ptr<ConnectionBuffer>)>
      handlers[UINT8_MAX + 1];

private:
  // Daca o sa am nevoie si de un position pointer sa dau wrap la asta intr-un
  // struct
  static thread_local uint8_t tls_buffer[std::numeric_limits<uint16_t>::max()];
};
