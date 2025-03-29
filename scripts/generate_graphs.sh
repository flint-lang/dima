#!/usr/bin/env sh

# This script first converts all test output files to csv format, and then collects all data from all tests to put said data together into one single csv file
root="$(cd "$(dirname "$0")" && cd .. && pwd)"

# First, lets create all raw c csv files
for file in $(ls "$root/test/results/test_outputs/c"); do
    echo "-- Converting '$root/test/results/test_outputs/c/$file' to csv..."
    "$root/scripts/convert_to_csv.sh" "$root/test/results/test_outputs/c/$file" c
done

# First, lets create all raw cpp csv files
for file in $(ls "$root/test/results/test_outputs/cpp"); do
    echo "-- Converting '$root/test/results/test_outputs/cpp/$file' to csv..."
    "$root/scripts/convert_to_csv.sh" "$root/test/results/test_outputs/cpp/$file"
done

# Then, collect the actual data we want (this is easier to do in C++ than in shell)
echo
echo "-- Building 'create_csv' binary..."
clang "$root/test/create_csv.cpp" -g -lstdc++ -std=c++17 -o "$root/test/create_csv"
echo

mkdir -p "$root/test/results/test_data/processed"

echo "-- Packing data for the 'memory-usage' graph..."
"$root/test/create_csv" memory-usage
echo "-- Packing data for the 'memory-usage-medium' graph..."
"$root/test/create_csv" memory-usage-medium

echo "-- Packing data for the 'alloc-time' graph..."
"$root/test/create_csv" alloc-time
echo "-- Packing data for the 'alloc-time-medium' graph..."
"$root/test/create_csv" alloc-time-medium

echo "-- Packing data for the 'dealloc-time' graph..."
"$root/test/create_csv" dealloc-time
echo "-- Packing data for the 'dealloc-time-medium' graph..."
"$root/test/create_csv" dealloc-time-medium

echo "-- Packing data for the 'simple-op' graph..."
"$root/test/create_csv" simple-op
echo "-- Packing data for the 'simple-op-medium' graph..."
"$root/test/create_csv" simple-op-medium

echo "-- Packing data for the 'complex-op' graph..."
"$root/test/create_csv" complex-op
echo "-- Packing data for the 'complex-op-medium' graph..."
"$root/test/create_csv" complex-op-medium

echo

# Check if 'gnuplot' is installed
if [ "$(which gnuplot)" = "" ]; then
    echo "Error: 'gnuplot' is not installed! It is needed to generate the plots from the benchmark data!"
    exit 1
fi

# $1 - The title of the graph
# $2 - The name of the csv file
# $3 - The label of the y axis
# $4 - Whether the graph is log (0 if not, 1 if yes)
create_graph() {
    gnuplot -c "$root/test/results/test_data/graphs/plot_template.gnuplot" \
        "$1" \
        "$3" \
        "$4" \
        "$root/test/results/test_data/processed/$2.csv" \
        "$root/test/results/test_data/graphs/$2.png"
}

echo "-- Creating the 'memory-usage' graph..."
create_graph "Memory Usage" memory-usage "Memory Usage in MB" 0

echo "-- Creating the 'memory-usage-medium' graph..."
create_graph "Memory Usage (Medium)" memory-usage-medium "Memory Usage in MB" 0

echo "-- Creating the 'memory-usage-reserve' graph..."
create_graph "Memory Usage (Reserve)" memory-usage-reserve "Memory Usage in MB" 0

echo "-- Creating the 'memory-usage-reserve-medium' graph..."
create_graph "Memory Usage (Reserve, Medium)" memory-usage-reserve-medium "Memory Usage in MB" 0

echo "-- Creating the 'alloc' graph..."
create_graph "Allocation Time" alloc "Time in ms" 1

echo "-- Creating the 'alloc-medium' graph..."
create_graph "Allocation Time (Medium)" alloc-medium "Time in ms" 1

echo "-- Creating the 'simple-ops' graph..."
create_graph "Simple Ops" simple-ops "Time in ms" 1

echo "-- Creating the 'simple-ops-medium' graph..."
create_graph "Simple Ops (Medium)" simple-ops-medium "Time in ms" 1

echo "-- Creating the 'complex-ops' graph..."
create_graph "Complex Ops" complex-ops "Time in ms" 1

echo "-- Creating the 'complex-ops-medium' graph..."
create_graph "Complex Ops (Medium)" complex-ops-medium "Time in ms" 1

echo "-- Creating the 'dealloc' graph..."
create_graph "Deallocation Time" dealloc "Time in ms" 1

echo "-- Creating the 'dealloc-medium' graph..."
create_graph "Deallocation TIme (Medium)" dealloc-medium "Time in ms" 1
