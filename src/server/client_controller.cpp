#include "client_controller.hpp"
#include "protocol.hpp"

std::function<void(std::shared_ptr<ConnectionBuffer>)> ClientController::handlers[UINT8_MAX + 1];

void ClientController::initialize()
{
    handlers[SERVER_STATUS] = [](std::shared_ptr<ConnectionBuffer> cb)
        {};

    handlers[CRC_VERIFY] = [](std::shared_ptr<ConnectionBuffer> cb)
        {};

    handlers[FILES_TRANSFER_START] = [](std::shared_ptr<ConnectionBuffer> cb)
        {};

    handlers[FILE_TRANSFER_CHUNK] = [](std::shared_ptr<ConnectionBuffer> cb)
        {};

    handlers[FILE_TRANSFER_END] = [](std::shared_ptr<ConnectionBuffer> cb)
        {};

    handlers[GET_SETTINGS] = [](std::shared_ptr<ConnectionBuffer> cb)
        {};

    handlers[SET_SETTING] = [](std::shared_ptr<ConnectionBuffer> cb)
        {};


}



void ClientController::process_packet(std::shared_ptr<ConnectionBuffer> cb)
{
    auto packet_type = cb->get_packet_type();
    handlers[packet_type](cb);
}
