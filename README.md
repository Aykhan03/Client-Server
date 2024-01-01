# Client-Server Communication System

## Overview
This project includes two C++ programs (`server_client_fifo.cpp` and `server_client_socket.cpp`) that implement basic client-server communication models using FIFOs and TCP/IP sockets, respectively.

### `server_client_fifo.cpp`
- **Mode of Operation**: Run in server mode with `-s` and in client mode with `-c`.
- **Client Requests**: Reads actions from a file and sends requests like GET, PUT, DELETE, GTIME, DELAY, QUIT.
- **Server Functionality**: Handles file operations and system time requests, and supports commands like 'list' and 'quit'.
- **Communication**: Uses FIFOs for inter-process communication.
- **Limitations**: Supports up to 3 clients, limited by the FIFO creation process.

### `server_client_socket.cpp`
- **Socket-Based Communication**: Implements client-server communication over a network using TCP/IP sockets.
- **Flexible Client-Server Interaction**: Handles various requests including file operations and system time inquiries.
- **Robust File Handling**: Enables file contents to be sent and received between clients and server.
- **Server Management**: Listens and manages multiple client connections efficiently.
- **Usage**: Server mode with `-s` and port number; client mode with `-c`, client ID, input file, server name, and port number.

## Getting Started
Before running the programs, ensure that the required FIFOs are created for `server_client_fifo.cpp` and that the network configuration is correctly set up for `server_client_socket.cpp`.

