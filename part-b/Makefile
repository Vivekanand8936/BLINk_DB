CXX = g++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -pthread
INCLUDES = -I.

TARGETS = blinkdb_server blinkdb_client benchmark

all: $(TARGETS)

blinkdb_server: src/main_server.cpp src/storage_engine.cpp src/network_server.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^

blinkdb_client: src/network_client.cpp src/storage_engine.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^

benchmark: benchmark.cpp src/storage_engine.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^

clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean
