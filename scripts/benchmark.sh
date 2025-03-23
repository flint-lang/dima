#!/usr/bin/env sh

rm -r ./out/*

echo "-- Building the executables..."
./scripts/build.sh

echo "-- Benchmarking 'dima'..."
./out/dima

echo "-- Benchmarking 'dima-o1'..."
./out/dima-o1

echo "-- Benchmarking 'dima-medium'..."
./out/dima-medium

echo "-- Benchmarking 'dima-medium-o1'..."
./out/dima-medium-o1

echo "-- Benchmarking 'std-shared'..."
./out/std-shared

echo "-- Benchmarking 'std-shared-o1'..."
./out/std-shared-o1

echo "-- Benchmarking 'std-shared-medium'..."
./out/std-shared-medium

echo "-- Benchmarking 'std-shared-medium-o1'..."
./out/std-shared-medium-o1

echo "-- Benchmarking 'std-unique'..."
./out/std-unique

echo "-- Benchmarking 'std-unique-o1'..."
./out/std-unique-o1

echo "-- Benchmarking 'std-unique-medium'..."
./out/std-unique-medium

echo "-- Benchmarking 'std-unique-medium-o1'..."
./out/std-unique-medium-o1
