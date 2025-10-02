#pragma once

#include "storage_engine.h"
#include <memory>
#include <string>
#include <unordered_map>

class Server {
public:
    Server();
    ~Server();

    void run();
    void stop();

private:
    static constexpr int PORT = 9001;
    static constexpr int LISTEN_BACKLOG = 128;
    static constexpr int MAX_EVENTS = 1024;

    int server_fd = -1;
    int kq = -1;
    bool should_stop = false;
    std::unique_ptr<StorageEngine> storage;
    std::unordered_map<int, std::string> client_buffers;

    void setup_server();
    void setup_kqueue();
    void set_nonblocking(int fd);
    void handle_new_connection();
    void handle_client_data(int client_fd);
    std::string process_command(const std::string& command);
    std::string encode_resp(const std::string& response);
}; 