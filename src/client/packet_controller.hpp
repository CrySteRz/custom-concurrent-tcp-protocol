#pragma once

#include "packets.hpp"
#include "protocol.hpp"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <optional>
#include <sys/types.h>

std::optional<Packet> create_set_setting_packet_wrapped(char* value, Setting s)
{
    SetSettingPacket p;
    p.header.command    = PacketType::REQ_SET_SETTING;
    p.header.total_size = sizeof(SetSettingPacket);

    switch(s)
    {
        case Setting::COMPRESSION_LEVEL:
        {
            int parsed = atoi(value);
            if((parsed > UINT8_MAX) || (parsed < 0))
            {
                printf("Value is out of bounds (0..255)\n");
                return {};
            }
            uint8_t* val = reinterpret_cast<uint8_t*>(&p.value);
            *val = parsed;
            return *reinterpret_cast<Packet*>(&p);
        }
        default:
        {
            printf("Invalid setting\n");
            return {};
        }

    }
}

class PacketController
{
public:
    static std::optional<Packet> create_set_setting_packet(char* str)
    {
        if(strncmp(str, "COMPRESSION_LEVEL=", 18))
        {
            return create_set_setting_packet_wrapped(str + 19, Setting::COMPRESSION_LEVEL);
        }
        printf("The correct syntax is set SETTING=value\ni.e. set COMPRESSION_LEVEL=2\n");

        return {};
    }

    static Packet create_sample_packet(PacketType command)
    {
        Packet packet;
        packet.header.command    = command;
        packet.header.version    = 0;
        packet.header.total_size = sizeof(PacketHeader);

        return packet;
    }

    static Packet create_open_file_packet(char* file_path)
    {
        Packet          p;
        PacketOpenFile* packet = reinterpret_cast<PacketOpenFile*>(&p);
        packet->header.command = PacketType::REQ_FILE_OPEN;
        packet->header.version = 0;
        strcpy(packet->path, file_path);
        packet->header.total_size = sizeof(PacketOpenFile);

        return p;
    }

    static Packet create_get_id_packet()
    {
        return create_sample_packet(PacketType::REQ_GET_CURRENT_USER);
    }

    static Packet create_logout_packet()
    {
        return create_sample_packet(PacketType::REQ_LOGOUT);
    }

    static Packet create_ping_packet()
    {
        return create_sample_packet(PacketType::REQ_PING);
    }

    static Packet create_server_status_packet()
    {
        return create_sample_packet(PacketType::REQ_SERVER_STATUS);
    }

    static Packet create_connections_status_packet()
    {
        return create_sample_packet(PacketType::REQ_CONNECTIONS_INFO);
    }

    static Packet create_get_settings_packet()
    {
        return create_sample_packet(PacketType::REQ_GET_SETTINGS);
    }

    static Packet create_ls_packet()
    {
        return create_sample_packet(PacketType::REQ_FILE_LIST);
    }

    static Packet create_login_packet(const char* username, const char* password)
    {
        Packet p;
        p.header.command = PacketType::REQ_LOGIN;
        PacketLogin* p2 = reinterpret_cast<PacketLogin*>(&p);
        p.header.total_size = sizeof(PacketHeader) + sizeof(p2->password) + sizeof(p2->username);
        strcpy(p2->username, username);
        strcpy(p2->password, password);
        return p;
    }


};
