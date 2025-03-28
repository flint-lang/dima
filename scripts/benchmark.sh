#!/usr/bin/env sh

rm -r ./out/*

echo "-- Building the executables..."
./scripts/build.sh

# $1 - Whether to benchmark 'c' or 'cpp'
# $2 - The name of the executable to benchmark
benchmark() {
    echo "-- Benchmarking '$2'..."
    touch ./test/results/test_outputs/"$1/$2".txt
    time ./out/"$1"/"$2" | tee ./test/results/test_outputs/"$1/$2".txt
}


mkdir -p ./test/results/test_outputs
mkdir -p ./test/results/test_outputs/cpp
mkdir -p ./test/results/test_outputs/c

if [ "$1" != "skip_cpp" ]; then
    benchmark cpp dima
    benchmark cpp dima-o1
    benchmark cpp dima-medium
    benchmark cpp dima-medium-o1
    benchmark cpp dima-reserve
    benchmark cpp dima-reserve-o1
    benchmark cpp dima-reserve-medium
    benchmark cpp dima-reserve-medium-o1
    benchmark cpp std-shared
    benchmark cpp std-shared-o1
    benchmark cpp std-shared-medium
    benchmark cpp std-shared-medium-o1
    benchmark cpp std-unique
    benchmark cpp std-unique-o1
    benchmark cpp std-unique-medium
    benchmark cpp std-unique-medium-o1
fi

benchmark c dima-c
benchmark c dima-c-o1
benchmark c dima-medium-c
benchmark c dima-medium-c-o1
benchmark c dima-reserve-c
benchmark c dima-reserve-c-o1
benchmark c dima-reserve-medium-c
benchmark c dima-reserve-medium-c-o1
benchmark c malloc-c
benchmark c malloc-c-o1
benchmark c malloc-medium-c
benchmark c malloc-medium-c-o1

./scripts/generate_graphs.sh
