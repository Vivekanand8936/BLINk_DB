#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <list>
#include <iostream>
#include <map>
#include <vector>

class StorageEngine {
public:
    StorageEngine();
    ~StorageEngine();

    bool set(const std::string& key, const std::string& value);
    std::string get(const std::string& key);
    bool del(const std::string& key);
    void clear();  // Clear all data from memory and disk
    void force_flush();  // Force flush write buffer
    size_t size() const;  // Get total number of entries (in memory + on disk)

private:
    static constexpr size_t MAX_KEY_SIZE = 256;
    static constexpr size_t MAX_VALUE_SIZE = 1024;
    static constexpr size_t MAX_CACHE_SIZE = 10000000;  // 10M entries max in memory
    static constexpr size_t BATCH_SIZE = 1000000;  // 1M entries to batch write
    static constexpr const char* DISK_DIR = "disk_storage";
    static constexpr const char* DATA_FILE = "data.dat";
    static constexpr const char* INDEX_FILE = "index.dat";

    struct DiskEntry {
        size_t offset;
        size_t size;
    };

    struct BatchEntry {
        std::string key;
        std::string value;
    };

    std::unordered_map<std::string, std::string> data_;
    std::list<std::string> access_order;  // Track access order for LRU eviction
    mutable std::mutex mutex_;  // Make mutex mutable for const methods
    size_t pending_writes = 0;  // Track number of pending writes
    std::map<std::string, DiskEntry> disk_index;  // Index for disk entries
    std::vector<BatchEntry> write_buffer;  // Buffer for batch writes

    void load_disk_index();
    void save_disk_index();
    void update_disk_index(const std::string& key, size_t offset, size_t size);
    void remove_from_disk_index(const std::string& key);
    void flush_write_buffer();
}; 