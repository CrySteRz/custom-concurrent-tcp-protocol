#pragma once
#include <memory>
#include <functional>

class ConnectionBuffer;

class ClientController
{
public:
    static void process_packet(std::shared_ptr<ConnectionBuffer> cb);
    static void initialize();

    static std::function<void(std::shared_ptr<ConnectionBuffer>)> handlers[UINT8_MAX + 1];
};
