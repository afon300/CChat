# CChat - Cross-Platform C Chat Application

CChat is a lightweight, multi-threaded chat application implemented in C. It provides a robust client-server architecture designed for real-time communication with multi-client support, dynamic coloring, and cross-platform compatibility (Windows and Linux).

## Key Features

- **Multi-Client Architecture:** The server handles multiple concurrent connections using a thread-per-client model.
- **Cross-Platform Portability:** Custom abstraction layer (`portability.h`) allows seamless compilation and execution on both Windows (Winsock2/Win32 Threads) and Linux (POSIX Sockets/Pthreads).
- **Thread-Safe UI:** Uses mutexes to prevent console output collisions between the message reception thread and the user input loop.
- **Dynamic User Identification:** Automatic assignment of unique terminal colors to each client for improved chat readability.
- **Professional Engineering Standards:**
  - Strict C11 compliance (`-std=c11`).
  - Robust error handling for all system calls (sockets, threads, memory allocation).
  - Automated integration testing via Python.

## Project Structure

```text
CChat/
├── bin/                # Compiled binaries (Server and Client)
├── obj/                # Compiled object files
├── tests/              # Automated integration tests
├── client.c / .h       # Client-side core logic
├── server.c            # Multi-threaded server implementation
├── main.c              # Client entry point and UI
├── portability.h       # OS abstraction layer (Windows/Linux)
├── Makefile            # Cross-platform build automation
└── README.md           # Project documentation
```

## Getting Started

### Prerequisites

- **Windows:** MinGW-w64 (GCC) or similar environment.
- **Linux:** GCC and Pthreads library.
- **Python 3.x:** Required only for running automated tests.

### Compilation

The project uses a cross-platform `Makefile` that detects your operating system automatically.

```powershell
# Build both server and client
make

# Clean build artifacts
make clean
```

### Running the Application

1. **Start the Server:**
    ```bash
    ./bin/server
    ```
   The server listens on port `8080` by default.

2. **Start the Client:**
   ```bash
   ./bin/main
   ```
   - Enter the server's IP (use `127.0.0.1` for local testing).
   - Enter your nickname.
   - Start chatting! Type `/exit` to disconnect.

## Testing

Automated integration tests ensure the server and client communicate correctly.

```powershell
make test
```

## Technical Implementation Details

- **Networking:** Utilizes TCP (SOCK_STREAM) for reliable message delivery.
- **Concurrency:** 
  - **Server:** Spawns a new thread for every incoming connection to handle asynchronous I/O.
  - **Client:** Uses a dedicated background thread for message reception to keep the UI responsive for user input.
- **Synchronization:** Mutexes (`CRITICAL_SECTION` on Windows, `pthread_mutex_t` on Linux) are used to protect shared client lists and synchronized console printing.

## License

This project is licensed under the Apache License, Version 2.0. See the [LICENSE](LICENSE) file for details.
