#include "network_server.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <cstring>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/event.h>
#include <unistd.h>
#include <arpa/inet.h>

#ifndef TCP_NODELAY
#define TCP_NODELAY 1
#endif

Server::Server() : storage(std::make_unique<StorageEngine>()) {
    setup_server();
    setup_kqueue();
}

Server::~Server() {
    stop();
}

void Server::setup_server() {
    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("Failed to set socket options: " + std::string(strerror(errno)));
    }

    // Set up address structure
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Try to bind socket
    int bind_attempts = 0;
    const int max_attempts = 5;
    while (bind_attempts < max_attempts) {
        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == 0) {
            break;
        }
        bind_attempts++;
        if (bind_attempts == max_attempts) {
            throw std::runtime_error("Failed to bind socket after " + std::to_string(max_attempts) + 
                                   " attempts: " + std::string(strerror(errno)));
        }
        std::cout << "Bind attempt " << bind_attempts << " failed, retrying..." << std::endl;
        sleep(1);
    }

    // Listen for connections
    if (listen(server_fd, LISTEN_BACKLOG) < 0) {
        throw std::runtime_error("Failed to listen on socket: " + std::string(strerror(errno)));
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    // Set server socket to non-blocking
    set_nonblocking(server_fd);
}

void Server::setup_kqueue() {
    kq = kqueue();
    if (kq < 0) {
        throw std::runtime_error("Failed to create kqueue");
    }

    // Add server socket to kqueue
    struct kevent ev;
    EV_SET(&ev, server_fd, EVFILT_READ, EV_ADD, 0, 0, nullptr);
    if (kevent(kq, &ev, 1, nullptr, 0, nullptr) < 0) {
        throw std::runtime_error("Failed to add server socket to kqueue");
    }
}

void Server::set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        throw std::runtime_error("Failed to get socket flags");
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        throw std::runtime_error("Failed to set socket to non-blocking");
    }
}

void Server::handle_new_connection() {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "Failed to accept connection" << std::endl;
        }
        return;
    }

    // Set TCP_NODELAY to disable Nagle's algorithm
    int flag = 1;
    if (setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) < 0) {
        std::cerr << "Failed to set TCP_NODELAY" << std::endl;
    }

    // Set send buffer size to 64KB
    int sendbuf = 65536;
    if (setsockopt(client_fd, SOL_SOCKET, SO_SNDBUF, &sendbuf, sizeof(sendbuf)) < 0) {
        std::cerr << "Failed to set send buffer size" << std::endl;
    }

    // Set client socket to non-blocking
    set_nonblocking(client_fd);

    // Add client socket to kqueue for reading
    struct kevent ev;
    EV_SET(&ev, client_fd, EVFILT_READ, EV_ADD, 0, 0, nullptr);
    if (kevent(kq, &ev, 1, nullptr, 0, nullptr) < 0) {
        std::cerr << "Failed to add client to kqueue" << std::endl;
        close(client_fd);
        return;
    }

    // Initialize client buffer
    client_buffers[client_fd] = "";
}

void Server::handle_client_data(int client_fd) {
    char buffer[4096];
    ssize_t bytes_read;

    while ((bytes_read = read(client_fd, buffer, sizeof(buffer))) > 0) {
        client_buffers[client_fd].append(buffer, bytes_read);
        
        // Process complete commands
        while (true) {
            // Find the first complete command
            size_t pos = client_buffers[client_fd].find("\r\n");
            if (pos == std::string::npos) break;
            
            std::string line = client_buffers[client_fd].substr(0, pos);
            client_buffers[client_fd] = client_buffers[client_fd].substr(pos + 2);
            
            // Handle RESP protocol
            if (line.empty()) continue;
            
            if (line[0] == '*') {
                // Array (multi-bulk) format
                int num_args = std::stoi(line.substr(1));
                std::vector<std::string> args;
                bool complete = true;
                
                for (int i = 0; i < num_args && complete; i++) {
                    // Each argument should be a bulk string
                    pos = client_buffers[client_fd].find("\r\n");
                    if (pos == std::string::npos) {
                        complete = false;
                        break;
                    }
                    
                    line = client_buffers[client_fd].substr(0, pos);
                    client_buffers[client_fd] = client_buffers[client_fd].substr(pos + 2);
                    
                    if (line[0] != '$') {
                        complete = false;
                        break;
                    }
                    
                    int len = std::stoi(line.substr(1));
                    if (len < 0) {
                        args.push_back("");
                        continue;
                    }
                    
                    if (client_buffers[client_fd].length() < size_t(len + 2)) {
                        complete = false;
                        break;
                    }
                    
                    args.push_back(client_buffers[client_fd].substr(0, len));
                    client_buffers[client_fd] = client_buffers[client_fd].substr(len + 2);
                }
                
                if (!complete) break;
                
                // Process the command
                if (!args.empty()) {
                    std::string command = args[0];
                    for (size_t i = 1; i < args.size(); i++) {
                        command += " " + args[i];
                    }
                    std::string response = process_command(command);
                    
                    // Send response
                    size_t total_sent = 0;
                    while (total_sent < response.length()) {
                        ssize_t sent = write(client_fd, 
                                          response.c_str() + total_sent,
                                          response.length() - total_sent);
                        
                        if (sent < 0) {
                            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                continue;
                            }
                            close(client_fd);
                            client_buffers.erase(client_fd);
                            return;
                        }
                        
                        total_sent += sent;
                    }
                }
            } else {
                // Simple string format (for backward compatibility)
                std::string response = process_command(line);
                
                // Send response
                size_t total_sent = 0;
                while (total_sent < response.length()) {
                    ssize_t sent = write(client_fd, 
                                      response.c_str() + total_sent,
                                      response.length() - total_sent);
                    
                    if (sent < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            continue;
                        }
                        close(client_fd);
                        client_buffers.erase(client_fd);
                        return;
                    }
                    
                    total_sent += sent;
                }
            }
        }
    }

    if (bytes_read == 0 || (bytes_read < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
        close(client_fd);
        client_buffers.erase(client_fd);
        return;
    }
}

std::string Server::process_command(const std::string& command) {
    std::istringstream iss(command);
    std::string cmd;
    iss >> cmd;
    
    // Convert command to uppercase for case-insensitive comparison
    std::string upper_cmd = cmd;
    for (char& c : upper_cmd) {
        c = std::toupper(c);
    }
    
    if (upper_cmd == "PING") {
        return "+PONG\r\n";
    }
    else if (upper_cmd == "SET") {
        std::string key, value;
        iss >> key >> value;
        if (key.empty() || value.empty()) {
            return "-ERR wrong number of arguments for 'set' command\r\n";
        }
        if (storage->set(key, value)) {
            return "+OK\r\n";
        }
        return "-ERR invalid key or value\r\n";
    }
    else if (upper_cmd == "GET") {
        std::string key;
        iss >> key;
        if (key.empty()) {
            return "-ERR wrong number of arguments for 'get' command\r\n";
        }
        std::string value = storage->get(key);
        if (value.empty()) {
            return "$-1\r\n";
        }
        return "$" + std::to_string(value.length()) + "\r\n" + value + "\r\n";
    }
    else if (upper_cmd == "DEL") {
        std::string key;
        iss >> key;
        if (key.empty()) {
            return "-ERR wrong number of arguments for 'del' command\r\n";
        }
        if (storage->del(key)) {
            return ":1\r\n";
        }
        return ":0\r\n";
    }
    else if (upper_cmd == "CLEAR" || upper_cmd == "FLUSHALL" || upper_cmd == "FLUSHDB") {
        storage->clear();
        return "+OK\r\n";
    }
    else if (upper_cmd == "EXIT") {
        // Stop the server gracefully
        should_stop = true;
        storage->stop_async_writer();
        return "+OK\r\n";
    }
    else {
        return "-ERR unknown command '" + cmd + "'\r\n";
    }
}

std::string Server::encode_resp(const std::string& response) {
    // If response is already encoded (starts with +, -, :, $, or *)
    if (!response.empty() && (response[0] == '+' || response[0] == '-' || 
        response[0] == ':' || response[0] == '$' || response[0] == '*')) {
        return response;
    }
    
    if (response == "NULL") {
        return "$-1\r\n";
    }
    else if (response == "PONG") {
        return "+PONG\r\n";
    }
    else if (response == "OK") {
        return "+OK\r\n";
    }
    else if (response.substr(0, 6) == "Error:") {
        return "-" + response + "\r\n";
    }
    else if (response.substr(0, 2) == "# ") {
        // Handle INFO command response
        return "$" + std::to_string(response.length()) + "\r\n" + response + "\r\n";
    }
    else {
        // Bulk string response
        return "$" + std::to_string(response.length()) + "\r\n" + response + "\r\n";
    }
}

void Server::run() {
    struct kevent events[MAX_EVENTS];
    
    while (!should_stop) {  // Check should_stop flag
        int nev = kevent(kq, nullptr, 0, events, MAX_EVENTS, nullptr);
        
        if (nev < 0) {
            if (errno == EINTR) continue;
            throw std::runtime_error("kevent error");
        }
        
        for (int i = 0; i < nev; i++) {
            int fd = events[i].ident;
            
            if (fd == server_fd) {
                // New connection
                handle_new_connection();
            }
            else {
                // Client data
                if (events[i].flags & EV_EOF) {
                    close(fd);
                    client_buffers.erase(fd);
                }
                else {
                    handle_client_data(fd);
                }
            }
        }
    }
    
    // Clean up when server stops
    stop();
}

void Server::stop() {
    // Close all client connections
    for (const auto& pair : client_buffers) {
        close(pair.first);
    }
    client_buffers.clear();
    
    // Close server socket
    if (server_fd >= 0) {
        close(server_fd);
        server_fd = -1;
    }
    
    // Close kqueue
    if (kq >= 0) {
        close(kq);
        kq = -1;
    }
} 