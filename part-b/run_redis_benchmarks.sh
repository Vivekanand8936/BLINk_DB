#!/bin/bash

# Create results directory if it doesn't exist
mkdir -p results

# Array of concurrent requests and parallel connections to test
requests=(1000000)  
connections=(100 1000)  

# Function to run redis-benchmark and save results
run_benchmark() {
    local num_requests=$1
    local num_connections=$2
    local output_file="results/result_${num_requests}_${num_connections}.txt"
    
    echo "Running redis-benchmark with $num_requests requests and $num_connections connections..."
    redis-benchmark -p 9001 -n $num_requests -c $num_connections > "$output_file"
    echo "Results saved to $output_file"
}

# Run benchmarks for all combinations
for req in "${requests[@]}"; do
    for conn in "${connections[@]}"; do
        run_benchmark $req $conn
    done
done

echo "All redis-benchmark tests completed!" 