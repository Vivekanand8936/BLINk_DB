#include "network_server.h"
#include <iostream>
#include <signal.h>

Server* g_server = nullptr;

void signal_handler(int signum) {
    if (g_server) {
        std::cout << "\nReceived signal " << signum << ", shutting down server..." << std::endl;
        g_server->stop();
    }
}

int main() {
    try {
        g_server = new Server();
        
        // Set up signal handlers
        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);
        
        std::cout << "Starting BLINK DB server on port 9001..." << std::endl;
        g_server->run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    delete g_server;
    return 0;
} 