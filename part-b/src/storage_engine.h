#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <list>
#include <iostream>
#include <map>
#include <vector>
#include <memory>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <fstream>
#include <filesystem>
#include <queue>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

class LRUCache {
private:
    struct Node {
        std::string key;
        std::string value;
        Node* prev;
        Node* next;
        Node(const std::string& k, const std::string& v) 
            : key(k), value(v), prev(nullptr), next(nullptr) {}
    };

    size_t capacity_;
    std::unordered_map<std::string, Node*> cache_;
    Node* head_;
    Node* tail_;
    std::mutex mutex_;

    void move_to_front(Node* node) {
        if (node == head_) return;
        
        if (node == tail_) {
            tail_ = node->prev;
            tail_->next = nullptr;
        } else {
            node->prev->next = node->next;
            node->next->prev = node->prev;
        }
        
        node->prev = nullptr;
        node->next = head_;
        head_->prev = node;
        head_ = node;
    }

    void evict_lru() {
        if (tail_) {
            cache_.erase(tail_->key);
            Node* temp = tail_;
            tail_ = tail_->prev;
            if (tail_) tail_->next = nullptr;
            delete temp;
        }
    }

public:
    LRUCache(size_t capacity) : capacity_(capacity), head_(nullptr), tail_(nullptr) {}
    
    ~LRUCache() {
        Node* current = head_;
        while (current) {
            Node* temp = current;
            current = current->next;
            delete temp;
        }
    }

    size_t capacity() const { return capacity_; }
    size_t size() const { return cache_.size(); }

    bool get(const std::string& key, std::string& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            move_to_front(it->second);
            value = it->second->value;
            return true;
        }
        return false;
    }

    bool put(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            it->second->value = value;
            move_to_front(it->second);
            return true;
        }

        if (cache_.size() >= capacity_) {
            evict_lru();
        }

        Node* new_node = new Node(key, value);
        cache_[key] = new_node;
        
        if (!head_) {
            head_ = tail_ = new_node;
        } else {
            new_node->next = head_;
            head_->prev = new_node;
            head_ = new_node;
        }
        
        return true;
    }

    bool remove(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            Node* node = it->second;
            if (node == head_) {
                head_ = node->next;
                if (head_) head_->prev = nullptr;
                else tail_ = nullptr;
            } else if (node == tail_) {
                tail_ = node->prev;
                if (tail_) tail_->next = nullptr;
            } else {
                node->prev->next = node->next;
                node->next->prev = node->prev;
            }
            cache_.erase(key);
            delete node;
            return true;
        }
        return false;
    }
};

class DiskStorage {
private:
    std::string data_file_;
    std::mutex mutex_;
    std::unordered_map<std::string, std::string> data_;

    std::filesystem::path get_executable_path() {
        #ifdef __APPLE__
            char path[1024];
            uint32_t size = sizeof(path);
            if (_NSGetExecutablePath(path, &size) == 0) {
                return std::filesystem::path(path).parent_path();
            }
        #else
            char result[PATH_MAX];
            ssize_t count = readlink("/proc/self/exe", result, sizeof(result));
            if (count != -1) {
                return std::filesystem::path(result).parent_path();
            }
        #endif
        return std::filesystem::current_path();
    }

    void ensure_directory_exists() {
        std::filesystem::path exe_path = get_executable_path();
        std::filesystem::path storage_dir = exe_path / "disk_storage";
        std::filesystem::create_directories(storage_dir);
    }

    void load_data() {
        std::ifstream file(data_file_);
        if (!file.is_open()) return;

        std::string line;
        while (std::getline(file, line)) {
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                data_[key] = value;
            }
        }
        file.close();
    }

    void save_data() {
        std::ofstream file(data_file_);
        if (!file.is_open()) return;

        for (const auto& [key, value] : data_) {
            file << key << "=" << value << "\n";
        }
        file.close();
    }

public:
    DiskStorage() {
        try {
            ensure_directory_exists();
            std::filesystem::path exe_path = get_executable_path();
            data_file_ = (exe_path / "disk_storage" / "data.txt").string();
            load_data();
        } catch (const std::exception& e) {
            // If loading fails, start with empty data
            data_.clear();
        }
    }

    ~DiskStorage() {
        save_data();
    }

    bool get(const std::string& key, std::string& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = data_.find(key);
        if (it != data_.end()) {
            value = it->second;
            return true;
        }
        return false;
    }

    bool put(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        data_[key] = value;
        save_data();
        return true;
    }

    void remove(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        data_.erase(key);
        save_data();
    }
};

class StorageEngine {
public:
    StorageEngine(size_t cache_size = 1) 
        : cache_(std::make_unique<LRUCache>(cache_size))
        , running_(false) {
        try {
            disk_storage_ = std::make_unique<DiskStorage>();
            running_ = true;
            write_thread_ = std::thread(&StorageEngine::async_write_worker, this);
        } catch (const std::exception& e) {
            running_ = false;
            throw;
        }
    }
    
    ~StorageEngine() {
        if (running_) {
            running_ = false;
            write_cv_.notify_one();
            if (write_thread_.joinable()) {
                write_thread_.join();
            }
        }
    }
    
    bool set(const std::string& key, const std::string& value) {
        return put(key, value);
    }
    
    std::string get(const std::string& key) {
        std::string value;
        if (get(key, value)) {
            return value;
        }
        return "";
    }
    
    bool get(const std::string& key, std::string& value) {
        // First check cache
        if (cache_->get(key, value)) {
            return true;
        }

        // If not in cache, check disk storage
        if (disk_storage_->get(key, value)) {
            // Add to cache
            cache_->put(key, value);
            return true;
        }
        return false;
    }
    
    bool del(const std::string& key) {
        try {
            // First check if key exists in either cache or disk
            std::string value;
            bool exists = false;
            
            // Check cache first
            if (cache_->get(key, value)) {
                exists = true;
                cache_->remove(key);
            }
            
            // Then check disk storage
            if (disk_storage_->get(key, value)) {
                exists = true;
                disk_storage_->remove(key);
            }
            
            return exists;
        } catch (const std::exception& e) {
            return false;
        }
    }
    
    void clear() {
        cache_ = std::make_unique<LRUCache>(cache_->capacity());
        disk_storage_ = std::make_unique<DiskStorage>();
    }
    
    void force_flush() {
        std::lock_guard<std::mutex> lock(write_mutex_);
        while (!write_queue_.empty()) {
            auto [key, value] = write_queue_.front();
            write_queue_.pop();
            disk_storage_->put(key, value);
        }
    }
    
    size_t size() const {
        return cache_->size();
    }
    
    void sync() {
        force_flush();
    }
    
    size_t pending_write_count() const {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(write_mutex_));
        return write_queue_.size();
    }
    
    void stop_async_writer() {
        running_ = false;
        write_cv_.notify_one();
        if (write_thread_.joinable()) {
            write_thread_.join();
        }
    }
    
    bool put(const std::string& key, const std::string& value) {
        // First try to put in cache
        if (cache_->put(key, value)) {
            // If successful, queue async write to disk
            {
                std::lock_guard<std::mutex> lock(write_mutex_);
                write_queue_.push({key, value});
            }
            write_cv_.notify_one();
            return true;
        }

        // If cache is full, write directly to disk
        return disk_storage_->put(key, value);
    }
    
private:
    void async_write_worker() {
        while (running_) {
            std::unique_lock<std::mutex> lock(write_mutex_);
            write_cv_.wait(lock, [this] { 
                return !running_ || !write_queue_.empty(); 
            });
            
            if (!running_) break;
            
            auto [key, value] = write_queue_.front();
            write_queue_.pop();
            lock.unlock();
            
            disk_storage_->put(key, value);
        }
    }
    
    std::unique_ptr<LRUCache> cache_;
    std::unique_ptr<DiskStorage> disk_storage_;
    std::queue<std::pair<std::string, std::string>> write_queue_;
    std::mutex write_mutex_;
    std::condition_variable write_cv_;
    std::thread write_thread_;
    std::atomic<bool> running_;
}; 