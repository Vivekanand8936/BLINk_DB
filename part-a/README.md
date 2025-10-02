# BLINK DB - Part 1: Storage Engine

This is the first part of the BLINK DB project, implementing a high-performance key-value storage engine.

## Features

- In-memory key-value storage
- Thread-safe operations
- Simple REPL interface
- Support for basic operations:
  - SET: Store a key-value pair
  - GET: Retrieve a value by key
  - DEL: Delete a key-value pair

## Building

### Prerequisites

- C++17 compatible compiler (g++ or clang++)
- Make
- Doxygen (for documentation)

### Build Instructions

1. Clone the repository
2. Navigate to the part-a directory
3. Run make:
   ```bash
   make
   ```

## Running

After building, run the REPL:
```bash
./blinkdb
```

### Example Usage

```
User> SET mykey myvalue
OK
User> GET mykey
myvalue
User> DEL mykey
OK
User> GET mykey
NULL
```

## Documentation

To generate documentation:
```bash
doxygen Doxyfile
```

The documentation will be generated in the `docs` directory in both HTML and PDF formats.

## Design

See `docs/design.md` for detailed design decisions and implementation details.

## Testing

The implementation has been tested with various workloads and concurrent access patterns. See the design document for more details about the testing strategy. 