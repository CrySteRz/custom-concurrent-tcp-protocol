#pragma once

#include "packets.hpp"
#include "protocol.hpp"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <optional>
#include <sys/types.h>
#include <sys/stat.h>
#include <vector>

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
            std::cout << "parsed:" << parsed << "\n";
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

    static Packet create_mkdir_packet(char* str)
    {
        Packet               p;
        PacketMakeDirectory* packet = reinterpret_cast<PacketMakeDirectory*>(&p);
        packet->header.command = PacketType::REQ_MKDIR;
        packet->header.version = 0;
        strcpy(packet->path, str);
        packet->header.total_size = sizeof(PacketMakeDirectory);

        return p;
    }

    static Packet create_compress_packet(Format format, Level compression_level, bool compress_all, const std::vector<std::string>& paths, const char* archive_name)
    {
        Packet          p;
        PacketCompress* packet = reinterpret_cast<PacketCompress*>(&p);
        packet->header.command    = PacketType::REQ_COMPRESS;
        packet->header.version    = 0;
        packet->format            = format;
        packet->compression_level = compression_level;
        packet->compress_all      = compress_all;
        packet->header.total_size = sizeof(PacketCompress);
        strcpy(packet->archive_name, archive_name);
        packet->file_count = paths.size();

        for(size_t i = 0; i < paths.size(); i++)
        {
            strcpy(packet->file_names[i], paths[i].c_str());
        }

        return p;
    }

    static Packet create_remove_path_packet(char* str)
    {
        Packet            p;
        PacketRemovePath* packet = reinterpret_cast<PacketRemovePath*>(&p);
        packet->header.command = PacketType::REQ_REMOVE_PATH;
        packet->header.version = 0;
        strcpy(packet->path, str);
        packet->header.total_size = sizeof(PacketRemovePath);

        return p;
    }

    static Packet create_sample_packet(PacketType command)
    {
        Packet packet;
        packet.header.command    = command;
        packet.header.version    = 0;
        packet.header.total_size = sizeof(PacketHeader);

        return packet;
    }

    static Packet create_download_file_packet(char* file_name)
    {
        Packet              p;
        PacketDownloadFile* packet = reinterpret_cast<PacketDownloadFile*>(&p);
        packet->header.command = PacketType::REQ_FILE_OPEN;
        packet->header.version = 0;
        strcpy(packet->path, file_name);
        packet->header.total_size = sizeof(PacketDownloadFile);

        return p;
    }

    static Packet create_mv_packet(char* path1, char* path2)
    {
        Packet          p;
        PacketMoveFile* packet = reinterpret_cast<PacketMoveFile*>(&p);
        packet->header.command = PacketType::REQ_MOVE_FILE;
        packet->header.version = 0;
        strcpy(packet->first_path, path1);
        strcpy(packet->second_path, path2);
        packet->header.total_size = sizeof(PacketMoveFile);

        return p;
    }

    static Packet create_cp_packet(char* path1, char* path2)
    {
        Packet          p;
        PacketCopyFile* packet = reinterpret_cast<PacketCopyFile*>(&p);
        packet->header.command = PacketType::REQ_COPY_FILE;
        packet->header.version = 0;
        strcpy(packet->first_path, path1);
        strcpy(packet->second_path, path2);
        packet->header.total_size = sizeof(PacketCopyFile);

        return p;
    }

    static Packet create_change_wd_packet(char* new_fp)
    {
        Packet                        p;
        PacketChangeCurrentDirectory* packet = reinterpret_cast<PacketChangeCurrentDirectory*>(&p);
        packet->header.command = PacketType::REQ_CHANGE_WORKING_DIRECTORY;
        packet->header.version = 0;
        strcpy(packet->path, new_fp);
        packet->header.total_size = sizeof(PacketChangeCurrentDirectory);

        return p;
    }

    static Packet create_get_cwd_packet()
    {
        return create_sample_packet(PacketType::REQ_CURRENT_DIRECTORY);
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
