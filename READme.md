# AsyncLogger

**AsyncLogger** is a high-performance, thread-safe, asynchronous logging system written in modern C++.  
It supports log levels, log rotation by file size, optional JSON format, colored console output, and file separation by severity.

## Features

- **Asynchronous logging**: avoids blocking the main thread
- **Multiple log levels**: DEBUG, INFO, WARNING, ERROR
- **Log rotation**: rotates logs when a file reaches a configurable size
- **Color-coded console output** (Linux/macOS)
- **File output**: with optional separation by log level
- **Optional JSON formatting** for structured logs
- **Modern C++**: uses smart pointers, `std::atomic`, `std::move`, and `std::emplace`