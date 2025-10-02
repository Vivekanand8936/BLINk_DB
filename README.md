# BLINK DB

High-performance key-value store with an in-memory storage engine (Part A) and a networked server that speaks the Redis protocol (Part B).

## Features

- In-memory and disk-backed storage engines
- Redis protocol (RESP) support in server mode
- Non-blocking I/O (kqueue/epoll), TCP_NODELAY for low latency
- Thread-safe LRU cache, async write buffering and batch writes
- Benchmarks and Doxygen documentation

## Repository Structure

```
.
├─ part-a/                 # Basic storage engine + REPL
│  ├─ src/
│  └─ disk_storage/
├─ part-b/                 # Network server, client, and benchmarks
│  ├─ src/
│  ├─ results/
│  └─ run_redis_benchmarks.sh
└─ docs/                   # Doxygen docs and mainpage
```

## Prerequisites

- C++17 compatible compiler (g++ or clang++)
- make
- pthread
- kqueue (macOS) or epoll (Linux)
- Doxygen (optional, for docs)

## Build

### Part A: Storage Engine
```bash
cd part-a
make clean
make
```

### Part B: Network Server
```bash
cd part-b
make clean
make
```

## Run

### Part A REPL
```bash
cd part-a
./blinkdb
```

### Part B Server and Client
```bash
cd part-b
./blinkdb_server

# in another terminal
cd part-b
./blinkdb_client
```

## Basic Commands (RESP)

- **SET**: `SET <key> <value>` → `+OK`
- **GET**: `GET <key>` → value or `$-1`
- **DEL**: `DEL <key>` → `:1` if deleted, `:0` otherwise
- **CLEAR/FLUSHDB/FLUSHALL** → `+OK`
- **PING** → `+PONG`
- **EXIT** → `+OK`

## Benchmarks

Run built-in script:
```bash
cd part-b
./run_redis_benchmarks.sh
```

Or using redis-benchmark:
```bash
redis-benchmark -p 9001 -n 100000 -c 100
```

Results are stored under `part-b/results/`.

## Configuration Highlights

Key parameters (see `storage_engine.h`):
- `MAX_CACHE_SIZE` – max in-memory entries
- `WRITE_BUFFER_SIZE` – write buffer size
- `BATCH_SIZE` – batch write size

## Documentation

Generate docs:
```bash
doxygen Doxyfile
```
Outputs to `docs/doxygen/html` and `docs/doxygen/latex`. See `docs/mainpage.md` for an overview.

## License

MIT (add a `LICENSE` file if applicable).
