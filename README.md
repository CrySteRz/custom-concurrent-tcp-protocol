# Server-Client Compression Service

This repository contains a client-server application that transfers files and directories to a remote server for compression using the Zstandard (zstd) library. The system utilizes a custom TCP protocol to ensure efficient and reliable communication and supports multiple client connections concurrently with a threadpool.

## Features

- **Concurrent Connection Management**: Handles multiple client connections simultaneously with a threadpool for improved scalability and performance.
- **Thread Safety**: Uses mutexes for safe concurrent access, ensuring data integrity.
- **Custom TCP Protocol**: Optimizes command and data transmission between the client and server.
- **Zstandard Compression**: Compresses files using the high-performance zstd algorithm.
- **Server Status**: Enables clients to verify the server's readiness for file transfers and compression.
- **File and Directory Transfer**: Supports transferring and compressing both individual files and entire directories.
- **Download Compressed Files**: Allows clients to download compressed files after server processing.
More coming...
