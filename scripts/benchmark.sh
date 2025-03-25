#!/usr/bin/env sh

rm -r ./out/*

echo "-- Building the executables..."
./scripts/build.sh

# $1 - The name of the executable to benchmark
benchmark() {
    echo "-- Benchmarking '$1'..."
    touch ./test/results/test_outputs/"$1".txt
    time ./out/"$1" | tee ./test/results/test_outputs/"$1".txt
}

mkdir -p "./test/results/test_outputs"

benchmark "dima"
benchmark "dima-o1"
benchmark "dima-medium"
benchmark "dima-medium-o1"
benchmark "dima-reserve"
benchmark "dima-reserve-o1"
benchmark "dima-reserve-medium"
benchmark "std-shared"
benchmark "std-shared-o1"
benchmark "std-shared-medium"
benchmark "std-shared-medium-o1"
benchmark "std-unique"
benchmark "std-unique-o1"
benchmark "std-unique-medium"
benchmark "std-unique-medium-o1"

./scripts/generate_graphs.sh
