#include <iostream>
#include <chrono>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <iomanip>
#include <thread>
#include <mutex>
#include <atomic>
#include <sstream>

class BenchmarkClient {
private:
    int sock;
    struct sockaddr_in server_addr;

public:
    BenchmarkClient(const char* host = "127.0.0.1", int port = 9001) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            throw std::runtime_error("Failed to create socket");
        }

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        if (inet_pton(AF_INET, host, &server_addr.sin_addr) <= 0) {
            throw std::runtime_error("Invalid address");
        }

        if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            throw std::runtime_error("Connection failed");
        }

        // Test connection with PING
        auto response = send_command("PING");
        if (response != "+PONG\r\n") {
            throw std::runtime_error("PING test failed");
        }
    }

    ~BenchmarkClient() {
        close(sock);
    }

    std::string send_command(const std::string& command) {
        std::vector<std::string> args;
        std::istringstream iss(command);
        std::string arg;
        while (iss >> arg) {
            args.push_back(arg);
        }
        
        // Format command in RESP protocol
        std::string resp_command = "*" + std::to_string(args.size()) + "\r\n";
        for (const auto& arg : args) {
            resp_command += "$" + std::to_string(arg.length()) + "\r\n" + arg + "\r\n";
        }
        
        if (send(sock, resp_command.c_str(), resp_command.length(), 0) < 0) {
            throw std::runtime_error("Send failed");
        }

        char buffer[1024] = {0};
        ssize_t received = recv(sock, buffer, sizeof(buffer), 0);
        if (received < 0) {
            throw std::runtime_error("Receive failed");
        }

        return std::string(buffer, received);
    }
};

struct BenchmarkResults {
    double set_ops_per_sec;
    double get_ops_per_sec;
};

BenchmarkResults run_client_benchmark(int num_operations) {
    BenchmarkResults results = {0.0, 0.0};
    try {
        BenchmarkClient client;
        
        // Benchmark SET operations
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < num_operations; i++) {
            std::string key = "key" + std::to_string(i);
            std::string value = "value" + std::to_string(i);
            std::string response = client.send_command("SET " + key + " " + value);
            if (response != "+OK\r\n") {
                throw std::runtime_error("SET operation failed");
            }
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        results.set_ops_per_sec = (num_operations * 1000000.0) / duration.count();
        
        // Benchmark GET operations
        start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < num_operations; i++) {
            std::string key = "key" + std::to_string(i);
            std::string response = client.send_command("GET " + key);
            std::string expected = "$" + std::to_string(std::string("value" + std::to_string(i)).length()) + 
                                 "\r\nvalue" + std::to_string(i) + "\r\n";
            if (response != expected) {
                throw std::runtime_error("GET operation failed");
            }
        }
        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        results.get_ops_per_sec = (num_operations * 1000000.0) / duration.count();
        
    } catch (const std::exception& e) {
        std::cerr << "Benchmark error: " << e.what() << std::endl;
    }
    return results;
}

void run_parallel_benchmark(int num_operations, int num_connections) {
    std::vector<std::thread> threads;
    std::atomic<double> total_set_ops{0.0};
    std::atomic<double> total_get_ops{0.0};
    
    // Calculate operations per thread
    int ops_per_thread = num_operations / num_connections;
    
    // Create and run threads
    for (int i = 0; i < num_connections; i++) {
        threads.emplace_back([ops_per_thread, &total_set_ops, &total_get_ops]() {
            auto results = run_client_benchmark(ops_per_thread);
            total_set_ops.store(total_set_ops.load() + results.set_ops_per_sec);
            total_get_ops.store(total_get_ops.load() + results.get_ops_per_sec);
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Print results
    std::cout << "====== BENCHMARK RESULTS ======" << std::endl;
    std::cout << "Number of operations: " << num_operations << std::endl;
    std::cout << "Number of parallel connections: " << num_connections << std::endl;
    std::cout << "Total SET operations per second: " << std::fixed << std::setprecision(2) << total_set_ops.load() << std::endl;
    std::cout << "Total GET operations per second: " << std::fixed << std::setprecision(2) << total_get_ops.load() << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <num_operations> <num_connections>" << std::endl;
        return 1;
    }
    
    int num_operations = std::stoi(argv[1]);
    int num_connections = std::stoi(argv[2]);
    
    try {
        run_parallel_benchmark(num_operations, num_connections);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 