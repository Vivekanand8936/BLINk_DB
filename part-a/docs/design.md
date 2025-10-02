# BLINK DB - Part 1 Design Document

## Overview
This document outlines the design decisions and implementation details for the storage engine component of BLINK DB.

## Design Decisions

### 1. Data Structure Choice
- **LRU Cache Implementation**: We use a combination of:
  - `std::unordered_map` for O(1) key lookups
  - `std::list` for maintaining LRU order
  - Custom `LRUNode` structure for storing key-value pairs
- **Benefits**:
  - O(1) average case complexity for all operations
  - Efficient LRU eviction policy
  - Thread-safe operations with mutex protection
  - Memory-efficient storage

### 2. Memory Management
- **LRU Eviction Policy**: 
  - When database reaches capacity (1 million entries)
  - Least recently used items are automatically evicted
  - Most recently used items are kept in memory
- **Memory Tracking**:
  - Maintains current size count
  - Enforces maximum size limit
  - Automatic cleanup of evicted items

### 3. Thread Safety
- **Mutex Protection**: All operations are protected by a single mutex to ensure thread safety
- **RAII Lock Management**: Using `std::lock_guard` for automatic mutex management

### 4. Performance Optimizations
- **O(1) Operations**: 
  - GET: O(1) lookup + O(1) list movement
  - SET: O(1) lookup + O(1) list insertion/update
  - DEL: O(1) lookup + O(1) list removal
- **Efficient LRU Updates**:
  - Using `std::list::splice` for O(1) list reordering
  - No unnecessary copying of data
- **Memory Efficiency**:
  - Automatic cleanup of unused items
  - No memory leaks
  - Efficient string handling

### 5. Workload Optimization
The implementation is optimized for a balanced workload because:
- All operations (GET, SET, DEL) have similar O(1) complexity
- LRU policy ensures frequently accessed items stay in memory
- Efficient handling of both reads and writes


### Advantages
1. Simple and efficient implementation
2. Predictable performance for all operations
3. Thread-safe operations
4. Memory-efficient storage with LRU eviction
5. Automatic memory management
6. No memory leaks

## Testing Strategy
1. Unit tests for individual operations
2. Stress testing with large datasets
3. Concurrent access testing
4. Memory usage monitoring
5. Performance benchmarking
6. LRU eviction testing
7. Cache hit/miss ratio monitoring 