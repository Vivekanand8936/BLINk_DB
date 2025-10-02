# BLINK DB Documentation

## Overview
BLINK DB is a high-performance key-value store implementation with both in-memory and disk-based storage capabilities. The project is divided into two main parts:

1. **Part A: Basic Storage Engine**
   - Simple in-memory storage with access order tracking
   - Basic disk-based persistence with binary format
   - Synchronous write operations
   - Simple disk index management
   - REPL (Read-Eval-Print Loop) interface

2. **Part B: Advanced Network Server**
   - Redis protocol (RESP) support
   - Non-blocking I/O using kqueue/epoll
   - High-concurrency client handling
   - Advanced LRU cache implementation
   - Asynchronous disk operations
   - Network client implementation

## Architecture

### Storage Engine (Part A)
The basic storage engine provides core functionality for storing and retrieving key-value pairs. It features:
- Simple in-memory hash map storage
- Access order tracking for basic LRU behavior
- Binary format disk persistence
- Synchronous write operations
- Simple disk index management
- REPL interface for interactive testing

### Advanced Storage Engine (Part B)
The advanced storage engine includes sophisticated caching and async operations:
- **LRUCache class**: Thread-safe LRU cache with doubly-linked list implementation
- **DiskStorage class**: Persistent storage with automatic directory management
- **StorageEngine class**: Main engine with async write worker thread
- Write buffering and batch operations
- Cross-platform executable path detection

### Network Server (Part B)
The network server implements the Redis protocol and provides:
- Non-blocking I/O using kqueue/epoll
- High-concurrency client handling
- RESP protocol implementation
- Signal handling for graceful shutdown
- Network client implementation for testing

## Performance Characteristics

### Benchmark Results
1. **Low Load (1000 requests, 10 connections)**
   - SET: 66,666 ops/sec
   - GET: 83,333 ops/sec

2. **High Load (100,000 requests, 100 connections)**
   - SET: 167,785 ops/sec
   - GET: 168,067 ops/sec

### Key Features
- High throughput under load
- Low latency for most operations
- Good scaling with concurrency
- Efficient resource utilization

## Getting Started

### Building the Project
```bash
# Part A: Storage Engine
cd part-a
make clean
make

# Part B: Network Server
cd part-b
make clean
make
```

### Running the Server
```bash
cd part-b
./blinkdb_server
```

### Running the Client
```bash
cd part-b
./blinkdb_client
```

### Running Benchmarks
```bash
# Using redis-benchmark
redis-benchmark -p 9001 -n 100000 -c 100

# Using built-in benchmark
./benchmark 100000 100
```

### Running Part A REPL
```bash
cd part-a
./blinkdb
```

## Implementation Differences

### Part A vs Part B Storage Engines

| Feature | Part A (Basic) | Part B (Advanced) |
|---------|----------------|-------------------|
| **Storage Format** | Binary format (data.dat, index.dat) | Text format (data.txt) |
| **Caching** | Simple access order tracking | Thread-safe LRU cache with doubly-linked list |
| **Write Operations** | Synchronous, immediate flush | Asynchronous with background worker thread |
| **Thread Safety** | Basic mutex protection | Advanced thread-safe design with condition variables |
| **Disk Management** | Manual directory creation | Automatic executable path detection |
| **Memory Management** | Simple hash map | Sophisticated cache with eviction policies |
| **Interface** | REPL command-line | Network server with RESP protocol |

### Key Classes in Part B
- **LRUCache**: Implements thread-safe LRU eviction with O(1) operations
- **DiskStorage**: Handles persistent storage with automatic path management
- **StorageEngine**: Main engine coordinating cache and disk operations
- **Server**: Network server with kqueue/epoll for high concurrency
- **NetworkClient**: Client implementation for testing and benchmarking

## Project Structure
```
.
+-- part-a/                 # Basic Storage Engine
|   +-- src/
|   |   +-- storage_engine.cpp    # Basic storage implementation
|   |   +-- storage_engine.h      # Basic storage header
|   |   +-- repl.cpp             # REPL interface
|   +-- disk_storage/            # Disk storage files
|   |   +-- data.dat
|   |   +-- index.dat
|   +-- blinkdb                  # Compiled executable
+-- part-b/                 # Advanced Network Server
|   +-- src/
|   |   +-- main_server.cpp      # Server main entry point
|   |   +-- network_server.cpp   # Network server implementation
|   |   +-- network_server.h     # Network server header
|   |   +-- network_client.cpp   # Network client implementation
|   |   +-- storage_engine.cpp   # Advanced storage implementation
|   |   +-- storage_engine.h     # Advanced storage header
|   +-- benchmark.cpp            # Performance benchmark tool
|   +-- disk_storage/            # Disk storage files
|   |   +-- data.txt
|   +-- blinkdb_server           # Compiled server executable
|   +-- blinkdb_client           # Compiled client executable
|   +-- benchmark                # Compiled benchmark executable
+-- docs/                   # Documentation
    +-- mainpage.md
    +-- doxygen/           # Generated documentation
        +-- html/
        +-- latex/
```

## API Documentation

### Basic Storage Engine API (Part A)
The basic storage engine provides the following main interfaces:
- `set(key, value)`: Store a key-value pair
- `get(key)`: Retrieve a value by key
- `del(key)`: Delete a key-value pair
- `clear()`: Remove all key-value pairs
- `force_flush()`: Force flush write buffer to disk
- `size()`: Get total number of entries

### Advanced Storage Engine API (Part B)
The advanced storage engine provides enhanced functionality:
- `set(key, value)`: Store a key-value pair (alias for put)
- `put(key, value)`: Store a key-value pair with async write
- `get(key)`: Retrieve a value by key
- `get(key, value)`: Retrieve a value by key (reference version)
- `del(key)`: Delete a key-value pair
- `clear()`: Remove all key-value pairs
- `force_flush()`: Force flush pending writes
- `sync()`: Synchronize all pending operations
- `size()`: Get cache size
- `pending_write_count()`: Get number of pending writes
- `stop_async_writer()`: Stop the async write worker

### LRUCache API (Part B)
The LRU cache provides thread-safe caching:
- `get(key, value)`: Retrieve value from cache
- `put(key, value)`: Store value in cache
- `remove(key)`: Remove key from cache
- `capacity()`: Get cache capacity
- `size()`: Get current cache size

### Network Server API (Part B)
The network server implements:
- RESP protocol parsing and encoding
- Client connection management with kqueue/epoll
- Command processing (SET, GET, DEL, PING, etc.)
- Response formatting
- Signal handling for graceful shutdown

### Network Client API (Part B)
The network client provides:
- TCP connection management
- RESP protocol communication
- Command sending and response parsing
- Error handling and connection testing

## Performance Optimization

### Memory Management (Part B)
- **LRU Cache**: Thread-safe doubly-linked list implementation for efficient cache management
- **Write Buffering**: Batch operations for better disk I/O performance
- **Memory-efficient Data Structures**: Optimized hash maps and linked lists
- **Async Write Worker**: Background thread for non-blocking disk operations

### Disk Operations
- **Part A**: Synchronous binary format with simple index management
- **Part B**: Asynchronous writes with background worker thread
- **Cross-platform Path Detection**: Automatic executable path detection for storage location
- **Batch Index Updates**: Efficient disk index management

### Network Optimization (Part B)
- **Non-blocking I/O**: kqueue/epoll for high-concurrency handling
- **RESP Protocol**: Efficient Redis-compatible protocol implementation
- **Signal Handling**: Graceful shutdown with SIGINT/SIGTERM support
- **Client Connection Management**: Efficient client buffer management

## Future Improvements

### Performance
- Implement connection pooling
- Add read-ahead caching
- Optimize disk index structure

### Features
- Add transaction support
- Implement pub/sub
- Add data compression

### Reliability
- Add data replication
- Implement checkpointing
- Add data validation 