#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdexcept>
#include <sstream>

class NetworkClient {
private:
    int sock;
    struct sockaddr_in server_addr;

    std::string parse_response(const std::string& resp) {
        std::istringstream iss(resp);
        char type;
        iss >> type;
        
        switch (type) {
            case '+': {  // Simple string
                std::string value;
                std::getline(iss, value);
                return value;
            }
            case '-': {  // Error
                std::string error;
                std::getline(iss, error);
                return "Error: " + error;
            }
            case ':': {  // Integer
                int value;
                iss >> value;
                return std::to_string(value);
            }
            case '$': {  // Bulk string
                int length;
                iss >> length;
                if (length == -1) return "NULL";
                std::string value;
                iss.ignore(2); // Skip \r\n
                std::getline(iss, value, '\r');
                return value;
            }
            default:
                return "Unknown response type";
        }
    }

public:
    NetworkClient(const char* host = "127.0.0.1", int port = 9001) {
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
    }

    ~NetworkClient() {
        close(sock);
    }

    std::string send_command(const std::string& command) {
        // Format command in RESP protocol
        std::string resp_command = "*1\r\n$" + std::to_string(command.length()) + "\r\n" + command + "\r\n";
        
        if (send(sock, resp_command.c_str(), resp_command.length(), 0) < 0) {
            throw std::runtime_error("Send failed");
        }

        char buffer[1024] = {0};
        ssize_t received = recv(sock, buffer, sizeof(buffer), 0);
        if (received < 0) {
            throw std::runtime_error("Receive failed");
        }

        return parse_response(std::string(buffer, received));
    }
};

int main(int argc, char* argv[]) {
    try {
        NetworkClient client;
        std::string command;
        
        std::cout << "Connected to BLINK DB server. Enter commands (EXIT to quit):\n";
        
        while (true) {
            std::cout << "User> ";
            std::getline(std::cin, command);
            
            if (command == "EXIT") {
                break;
            }
            
            if (command.empty()) {
                continue;
            }
            
            try {
                std::string response = client.send_command(command);
                std::cout << response << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << std::endl;
                break;
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 