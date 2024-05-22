#pragma once

#include "packets.hpp"
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <string>
#include <vector>
inline std::vector<std::string> ssplit(const std::string& str, char delimiter)
{
    std::vector<std::string> tokens;
    std::string              token;
    std::istringstream       tokenStream(str);

    while(std::getline(tokenStream, token, delimiter))
    {
        tokens.push_back(token);
    }

    return tokens;
}

//Function to join vector of strings with a delimiter
inline std::string sjoin(const std::vector<std::string>& tokens, char delimiter)
{
    std::ostringstream oss;

    for(size_t i = 0; i < tokens.size(); ++i)
    {
        if(i != 0)
        {
            oss << delimiter;
        }
        oss << tokens[i];
    }

    return oss.str();
}

//Function to modify the path based on the given destination
inline std::string cd_command(const std::string& current_path, const std::string& destination)
{
    std::vector<std::string> path_parts = ssplit(current_path, '/');

    //Split destination by '/'
    std::vector<std::string> dest_parts = ssplit(destination, '/');

    for(const auto& part : dest_parts)
    {
        if(part == "..")
        {
            //Move up one directory
            if(path_parts.size() > 3) //Ensure we don't move out of "./tmp/USERID"
            {
                path_parts.pop_back();
            }
        }
        else if(!part.empty() && (part != "."))
        {
            //Move into a specified directory
            path_parts.push_back(part);
            std::string new_path = sjoin(path_parts, '/');

            //Check if the new path exists and is a directory
            if(!std::filesystem::exists(new_path) || !std::filesystem::is_directory(new_path))
            {
                return {};
            }
        }
    }

    return sjoin(path_parts, '/');
}

inline void write_user_file_list(const char* users_dir, uint8_t* buffer)
{
    auto& p   = *reinterpret_cast<PacketFileList*>(buffer);
    DIR*  dir = opendir(users_dir); //User has no files or dir
    if(dir == nullptr)
    {
        p.file_count = 0;
        return;
    }

    struct dirent* entry;
    size_t         count = 0;

    while((entry = readdir(dir)) != nullptr && count < MAX_FILES)
    {
        if((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0))
        {
            continue;
        }

        strncpy(p.file_names[count], entry->d_name, MAX_FILENAME_LEN);
        strncpy(p.file_names[count], entry->d_name, MAX_FILENAME_LEN);
        p.file_names[count][MAX_FILENAME_LEN - 1] = '\0';

        ++count;
    }

    p.file_count = static_cast<uint8_t>(count);

    closedir(dir);
}

inline void make_socket_non_blocking(int socket_fd)
{
    int flags = fcntl(socket_fd, F_GETFL, 0);
    if(fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        perror("Cannot make nonblocking");
    }
}

inline bool is_valid_filename(const char* file_path)
{
    //Check if the file_path contains any path delimiters
    if((strstr(file_path, "../") != NULL)
        || (strchr(file_path, '/') != NULL)
        || (strchr(file_path, '\\') != NULL))
    {
        return false;
    }
    return true;
}

inline bool remove_file(const char* file_name)
{
    return std::remove(file_name) == 0;
}

inline bool remove_directory(const char* directory_name)
{
    return std::filesystem::remove_all(directory_name) > 0;
}

#include <filesystem>
namespace fs = std::filesystem;
inline bool is_path_within_user_dir(const fs::path& path, const fs::path& user_dir)
{
    auto canonical_path     = fs::weakly_canonical(path);
    auto canonical_user_dir = fs::weakly_canonical(user_dir);
    return canonical_path.string().find(canonical_user_dir.string()) == 0;
}

inline bool move_file(const fs::path& source, const fs::path& destination)
{
    try
    {
        fs::rename(source, destination);
        return true;
    }
    catch(fs::filesystem_error& e)
    {
        return false;
    }
}
