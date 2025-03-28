#!/usr/bin/env sh

rm -r ./out/*

echo "-- Building the executables..."
./scripts/build.sh

# $1 - The name of the executable to benchmark
benchmark_cpp() {
    echo "-- Benchmarking '$1'..."
    touch ./test/results/test_outputs/"$1".txt
    time ./out/"$1" | tee ./test/results/test_outputs/"$1".txt
}

mkdir -p "./test/results/test_outputs"

benchmark_cpp "dima"
benchmark_cpp "dima-o1"
benchmark_cpp "dima-medium"
benchmark_cpp "dima-medium-o1"
benchmark_cpp "dima-reserve"
benchmark_cpp "dima-reserve-o1"
benchmark_cpp "dima-reserve-medium"
benchmark_cpp "std-shared"
benchmark_cpp "std-shared-o1"
benchmark_cpp "std-shared-medium"
benchmark_cpp "std-shared-medium-o1"
benchmark_cpp "std-unique"
benchmark_cpp "std-unique-o1"
benchmark_cpp "std-unique-medium"
benchmark_cpp "std-unique-medium-o1"

./scripts/generate_graphs.sh
