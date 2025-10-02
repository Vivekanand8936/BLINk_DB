#include "storage_engine.h"
#include <iostream>
#include <sstream>
#include <string>

void printUsage() {
    std::cout << "Available commands:\n"
              << "1. SET <key> <value> - Set a key-value pair\n"
              << "2. GET <key> - Get value for a key\n"
              << "3. DEL <key> - Delete a key-value pair\n"
              << "4. SIZE - Get current size of database\n"
              << "5. CLEAR - Clear all data\n"
              << "6. EXIT - Exit the program\n";
}

int main() {
    StorageEngine db;
    std::string line;
    std::cout << "BLINK DB REPL\n";
    printUsage();
    
    while (true) {
        std::cout << "\nUser> ";
        std::getline(std::cin, line);
        
        if (line.empty()) {
            continue;
        }
        
        std::istringstream iss(line);
        std::string command;
        iss >> command;
        
        if (command == "EXIT") {
            break;
        }
        else if (command == "SET") {
            std::string key, value;
            iss >> key;
            std::getline(iss, value);
            
            // Remove leading space from value
            if (!value.empty() && value[0] == ' ') {
                value = value.substr(1);
            }
            
            if (key.empty() || value.empty()) {
                std::cout << "Error: SET requires both key and value\n";
                continue;
            }
            
            if (db.set(key, value)) {
                std::cout << "OK\n";
            } else {
                std::cout << "Error: Database is full\n";
            }
        }
        else if (command == "GET") {
            std::string key;
            iss >> key;
            
            if (key.empty()) {
                std::cout << "Error: GET requires a key\n";
                continue;
            }
            
            std::string value = db.get(key);
            std::cout << value << "\n";
        }
        else if (command == "DEL") {
            std::string key;
            iss >> key;
            
            if (key.empty()) {
                std::cout << "Error: DEL requires a key\n";
                continue;
            }
            
            if (db.del(key)) {
                std::cout << "OK\n";
            } else {
                std::cout << "Error: Key does not exist\n";
            }
        }
        else if (command == "SIZE") {
            std::cout << db.size() << "\n";
        }
        else if (command == "CLEAR") {
            db.clear();
            std::cout << "OK\n";
        }
        else {
            std::cout << "Unknown command. Type 'help' for usage.\n";
        }
    }
    
    return 0;
} 