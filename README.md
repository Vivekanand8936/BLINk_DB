# BLINK DB Project

A high-performance key-value store implementation with both in-memory and disk-based storage capabilities.

## Project Structure

- `part-a/`: In-memory storage engine implementation
- `part-b/`: Network server implementation with Redis protocol support

## Prerequisites

- C++17 compatible compiler (g++ or clang++)
- Make
- pthread library
- kqueue (macOS) or epoll (Linux) support
- Doxygen (for documentation)

## Building the Project

### Part A: Storage Engine
```bash
cd part-a
make clean
make
```
./blinkdb

### Part B: Network Server
```bash
cd part-b
make clean
make
```

## Running the Project
### Part B: Network Server
```bash
cd part-b
./blinkdb_server
```
cd part-b 
./blinkdb_client
...

### Basic Operations

1. **SET**
   ```bash
   SET <key> <value>
   ```
   Sets a key-value pair in the database.
   - Returns: `+OK` on success, `-ERR` on failure
   - Example: `SET name John`

2. **GET**
   ```bash
   GET <key>
   ```
   Retrieves the value associated with a key.
   - Returns: The value if found, `$-1` if not found
   - Example: `GET name`

3. **DEL**
   ```bash
   DEL <key>
   ```
   Deletes a key-value pair from the database.
   - Returns: `:1` if deleted, `:0` if not found
   - Example: `DEL name`

### Database Management

4. **CLEAR**
   ```bash
   CLEAR
   ```
   Removes all keys from the database.
   - Returns: `+OK`
   - Example: `CLEAR`

5. **FLUSHDB**
   ```bash
   FLUSHDB
   ```
   Alias for CLEAR. Removes all keys from the database.
   - Returns: `+OK`
   - Example: `FLUSHDB`

6. **FLUSHALL**
   ```bash
   FLUSHALL
   ```
   Alias for CLEAR. Removes all keys from the database.
   - Returns: `+OK`
   - Example: `FLUSHALL`

### Server Management

7. **PING**
   ```bash
   PING
   ```
   Tests the server connection.
   - Returns: `+PONG`
   - Example: `PING`

8. **EXIT**
   ```bash
   EXIT
   ```
   Gracefully shuts down the server.
   - Returns: `+OK`
   - Example: `EXIT`

## Testing

### Running Benchmarks
```bash
cd part-b
./run_redis_benchmarks.sh
```

### Using redis-benchmark
```bash
redis-benchmark -p 9001 -n 10000 -c 100
```

## Performance Features

- Asynchronous write operations
- Write buffering with configurable batch size
- LRU cache with configurable size
- Non-blocking I/O using kqueue/epoll
- TCP_NODELAY enabled for reduced latency
- Optimized disk index management

## Configuration

Key configuration parameters in `storage_engine.h`:
- `MAX_CACHE_SIZE`: Maximum number of entries in memory cache (default: 1M)
- `WRITE_BUFFER_SIZE`: Size of write buffer (default: 100K)
- `BATCH_SIZE`: Number of entries to batch write (default: 50K)

## Error Handling

The server implements proper error handling for:
- Invalid commands
- Missing arguments
- Connection issues
- Disk I/O errors
- Memory constraints

## Protocol

The server implements a subset of the Redis protocol (RESP):
- Simple strings: `+OK\r\n`
- Errors: `-ERR message\r\n`
- Integers: `:123\r\n`
- Bulk strings: `$length\r\nstring\r\n`
- Arrays: `*length\r\n$length\r\nstring\r\n`

## Documentation

To generate documentation:
```bash
doxygen Doxyfile
```

The documentation will be generated in the `docs` directory in both HTML and PDF formats.

## Performance Results

Performance results for different workloads are stored in the `results` directory:
- `result_10000_10.txt`: 10,000 requests, 10 connections
- `result_100000_100.txt`: 100,000 requests, 100 connections
- `result_1000000_1000.txt`: 1,000,000 requests, 1000 connections
