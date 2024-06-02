# Concurrential Programming Project

## Introduction
C++ application using a custom TCP protocol for file transfer and compression. It features SQLite-based authentication and supports various file and directory operations, all of which have custom implementations.

## Features
- Custom protocol over TCP sockets
- SQLite for user login
- File transfer: upload, download, copy, move, delete
- Directory management: create, list
- Compression: multiple formats and levels
- Admin commands for server status and settings

## User Commands
- **cwd**: Get current working directory
- **download <path>**: Download file from server
- **rm <path>**: Remove file/directory
- **mkdir <path>**: Create new directory
- **compress <archive> <options>**: Compress files (options: `--level`, `--format`)
- **cd <path>**: Change directory
- **mv <source> <destination>**: Move/rename file or directory
- **cp <source> <destination>**: Copy file or directory
- **getid**: Get session identifier
- **logout**: Logout from session
- **login <username> <password>**: Login to server
- **upload <file paths>**: Upload files
- **ls**: List files in current directory
- **ping**: Check server connectivity
- **help**: Display help message

## Admin Commands
- **status**: Get server status
- **get_settings**: Retrieve current settings
- **set <setting>**: Update settings
- **connections**: Display active/pending connections
- **clean**: Clean server by removing all files and directories

## Database Management
The `utils` folder contains scripts for creating and interacting with the SQLite database. These scripts are essential for initializing and maintaining the database structure used for user authentication.

