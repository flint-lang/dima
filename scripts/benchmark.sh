#!/usr/bin/env sh

echo "-- Building the executables..."
./scripts/build.sh

echo "-- Benchmarking 'dima' without optimizations..."
./out/dima

echo "-- Benchmarking 'dima' with '-O1' optimizations..."
./out/dima-o1

echo "-- Benchmarking 'std' without optimizations..."
./out/std

echo "-- Benchmarking 'std' with '-O1' optimizations..."
./out/std-o1
