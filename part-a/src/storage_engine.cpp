#include "storage_engine.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <vector>
#include <sstream>

StorageEngine::StorageEngine() {
    // Create disk_storage directory if it doesn't exist
    if (system("mkdir -p disk_storage") != 0) {
        std::cerr << "Failed to create disk_storage directory" << std::endl;
    }

    // Initialize empty state
    data_.clear();
    access_order.clear();
    pending_writes = 0;
    write_buffer.clear();
    disk_index.clear();

    // Load existing data
    load_disk_index();
}

StorageEngine::~StorageEngine() {
    force_flush();  // Force flush any remaining data
    save_disk_index();
}

size_t StorageEngine::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.size() + disk_index.size();
}

void StorageEngine::load_disk_index() {
    std::ifstream infile("disk_storage/index.dat", std::ios::binary);
    if (!infile.is_open()) {
        // Create empty files if they don't exist
        std::ofstream data_file("disk_storage/data.dat", std::ios::app);
        std::ofstream index_file("disk_storage/index.dat", std::ios::app);
        data_file.close();
        index_file.close();
        return;
    }

    while (infile) {
        uint32_t key_len;
        size_t offset, size;
        infile.read(reinterpret_cast<char*>(&key_len), sizeof(key_len));
        if (!infile) break;

        std::string key(key_len, '\0');
        infile.read(&key[0], key_len);
        infile.read(reinterpret_cast<char*>(&offset), sizeof(offset));
        infile.read(reinterpret_cast<char*>(&size), sizeof(size));

        if (infile) {
            disk_index[key] = {offset, size};
            
            // Also load into memory cache
            std::ifstream data_file("disk_storage/data.dat", std::ios::binary);
            if (data_file) {
                data_file.seekg(offset);
                
                // Skip key
                uint32_t stored_key_len;
                data_file.read(reinterpret_cast<char*>(&stored_key_len), sizeof(stored_key_len));
                data_file.seekg(stored_key_len, std::ios::cur);
                
                // Read value
                uint32_t value_len;
                data_file.read(reinterpret_cast<char*>(&value_len), sizeof(value_len));
                std::string value(value_len, '\0');
                data_file.read(&value[0], value_len);
                
                if (data_file) {
                    data_[key] = value;
                    access_order.push_back(key);
                }
            }
        }
    }
}

void StorageEngine::save_disk_index() {
    std::ofstream outfile("disk_storage/index.dat", std::ios::binary | std::ios::trunc);
    if (!outfile.is_open()) {
        std::cerr << "Failed to open index file for writing" << std::endl;
        return;
    }

    for (const auto& [key, entry] : disk_index) {
        uint32_t key_len = key.length();
        outfile.write(reinterpret_cast<const char*>(&key_len), sizeof(key_len));
        outfile.write(key.c_str(), key_len);
        outfile.write(reinterpret_cast<const char*>(&entry.offset), sizeof(entry.offset));
        outfile.write(reinterpret_cast<const char*>(&entry.size), sizeof(entry.size));
    }
}

void StorageEngine::update_disk_index(const std::string& key, size_t offset, size_t size) {
    disk_index[key] = {offset, size};
    save_disk_index();  // Save index after each update
}

void StorageEngine::remove_from_disk_index(const std::string& key) {
    disk_index.erase(key);
    save_disk_index();  // Save index after each removal
}

void StorageEngine::flush_write_buffer() {
    if (write_buffer.empty()) return;

    std::ofstream outfile("disk_storage/data.dat", std::ios::binary | std::ios::app);
    if (!outfile.is_open()) {
        std::cerr << "Failed to open data file for writing" << std::endl;
        return;
    }

    for (const auto& entry : write_buffer) {
        std::streampos offset = outfile.tellp();
        
        // Write key length and key
        uint32_t key_len = entry.key.length();
        outfile.write(reinterpret_cast<const char*>(&key_len), sizeof(key_len));
        outfile.write(entry.key.c_str(), key_len);
        
        // Write value length and value
        uint32_t value_len = entry.value.length();
        outfile.write(reinterpret_cast<const char*>(&value_len), sizeof(value_len));
        outfile.write(entry.value.c_str(), value_len);
        
        std::streampos end_pos = outfile.tellp();
        size_t size = static_cast<size_t>(end_pos - offset);
        update_disk_index(entry.key, static_cast<size_t>(offset), size);
    }
    
    outfile.close();
    write_buffer.clear();
    pending_writes = 0;
}

bool StorageEngine::set(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Add to memory cache
    data_[key] = value;
    access_order.push_back(key);
    pending_writes++;

    // Add to write buffer
    write_buffer.push_back({key, value});

    // Check if we need to evict entries
    if (data_.size() > MAX_CACHE_SIZE) {
        // Remove 20% of oldest entries
        size_t entries_to_remove = MAX_CACHE_SIZE / 5;
        for (size_t i = 0; i < entries_to_remove && !access_order.empty(); ++i) {
            std::string oldest_key = access_order.front();
            access_order.pop_front();
            data_.erase(oldest_key);
        }
    }
    
    // Flush write buffer immediately for better persistence
    flush_write_buffer();

    return true;
}

std::string StorageEngine::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);

    // First check memory cache
    auto it = data_.find(key);
    if (it != data_.end()) {
        // Update access order
        access_order.remove(key);
        access_order.push_back(key);
        return it->second;
    }

    // If not in memory, check disk index
    auto disk_it = disk_index.find(key);
    if (disk_it != disk_index.end()) {
        std::ifstream infile("disk_storage/data.dat", std::ios::binary);
        if (!infile.is_open()) {
            std::cerr << "Failed to open data file for reading" << std::endl;
            return "";
        }

        infile.seekg(disk_it->second.offset);
        if (!infile) {
            std::cerr << "Failed to seek to offset " << disk_it->second.offset << std::endl;
            return "";
        }
        
        // Read key length and key
        uint32_t key_len;
        infile.read(reinterpret_cast<char*>(&key_len), sizeof(key_len));
        if (!infile || key_len > MAX_KEY_SIZE) {
            std::cerr << "Failed to read key length or invalid key length" << std::endl;
            return "";
        }

        std::string stored_key(key_len, '\0');
        infile.read(&stored_key[0], key_len);
        if (!infile || stored_key != key) {
            std::cerr << "Failed to read key or key mismatch" << std::endl;
            return "";
        }
        
        // Read value length and value
        uint32_t value_len;
        infile.read(reinterpret_cast<char*>(&value_len), sizeof(value_len));
        if (!infile || value_len > MAX_VALUE_SIZE) {
            std::cerr << "Failed to read value length or invalid value length" << std::endl;
            return "";
        }

        std::string value(value_len, '\0');
        infile.read(&value[0], value_len);
        if (!infile) {
            std::cerr << "Failed to read value" << std::endl;
            return "";
        }
        
        // Add back to cache
        data_[key] = value;
        access_order.push_back(key);
        return value;
    }

    return "";
}

bool StorageEngine::del(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if key exists
    bool exists = data_.find(key) != data_.end() || disk_index.find(key) != disk_index.end();
    if (!exists) return false;

    // Remove from memory cache
    data_.erase(key);
    access_order.remove(key);

    // Remove from disk index
    remove_from_disk_index(key);

    return true;
}

void StorageEngine::clear() {
    std::lock_guard<std::mutex> lock(mutex_);

    // Clear memory data
    data_.clear();
    access_order.clear();
    pending_writes = 0;
    write_buffer.clear();
    disk_index.clear();

    // Clear disk files
    std::ofstream data_file("disk_storage/data.dat", std::ios::trunc);
    std::ofstream index_file("disk_storage/index.dat", std::ios::trunc);
    data_file.close();
    index_file.close();
}

void StorageEngine::force_flush() { 
    flush_write_buffer(); 
} 