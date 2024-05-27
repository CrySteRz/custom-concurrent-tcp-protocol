#include "connection.hpp"
#include "constants.hpp"
#include "state.hpp"
#include <atomic>
#include <dirent.h>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

ConnectionWrapper::~ConnectionWrapper()
{
    if(is_admin)
    {
        std::cout << "An admin disconnected!\n";
        g_state.b_admin_connected.exchange(false);
    }
    close(fd);
    close(download_fd);
    char dir_path[255];
    sprintf(dir_path, "%s/%s", SERVER_FILES_DIR, id.c_str());
    DIR* dir = opendir(dir_path);
    if(dir)
    {
        closedir(dir);
        //Cleanup the directory
        //std::filesystem::remove_all(dir_path); // Uncomment if you're using C++17 and have included <filesystem>
    }
}

void ConnectionWrapper::send_response_sync(uint8_t* buffer, size_t length)
{
    int sent_bytes = 0;

    while(true)
    {
        int result = send(this->socket_fd, buffer + sent_bytes, length - sent_bytes, 0);
        if(result == -1)
        {
            if((errno == EWOULDBLOCK) || (errno == EAGAIN))
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            perror("send");
            return;
        }
        sent_bytes += result;
        if(sent_bytes >= length)
        {
            return;
        }
    }
}

PacketType ConnectionBuffer::get_packet_type()
{
    if((connection->buffer[1] < 0) || (connection->buffer[1] > (int)PacketType::RESP_FILE_LIST))
    {
        return PacketType::REQ_SET_SETTING;
    }
    return (PacketType)connection->buffer[1];
}
