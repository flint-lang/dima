#!/usr/bin/env sh

# $1 - file_name
# $2 - binary_name
# $N - additional flags (for example -O1)
build_cpp() {
    file_name="$1"
    binary_name="$2"
    shift 2
    clang ./test/src/"$file_name" -o ./out/cpp/"$binary_name" -lstdc++ -lm -g -std=c++17 -I./ -Itest/include $@
}

# $1 - file_name
# $2 - binary_name
# $N - additional flags (for example -O1)
build_c() {
    file_name="$1"
    binary_name="$2"
    shift 2
    clang ./test/src/"$file_name" -o ./out/c/"$binary_name" -lc -lm -g -I./ -Itest/include $@
}

mkdir -p ./out/c
mkdir -p ./out/cpp

echo "-- Building the C test binaries..."

echo "-- Building 'dima-c'..."
build_c dima.c dima-c
echo "-- Building 'dima-medium-c'..."
build_c dima.c dima-medium-c -DMEDIUM_TEST
echo "-- Building 'dima-reserve-c'..."
build_c dima.c dima-reserve-c -DDIMA_RESERVE
echo "-- Building 'dima-reserve-medium-c'..."
build_c dima.c dima-reserve-medium-c -DMEDIUM_TEST -DDIMA_RESERVE

echo "-- Building 'malloc-c'..."
build_c dima.c malloc-c "-DRUN_MALLOC_TEST"
echo "-- Building 'malloc-medium-c'..."
build_c dima.c malloc-medium-c -DRUN_MALLOC_TEST -DMEDIUM_TEST

echo "-- Building 'dima-c-o1'..."
build_c dima.c dima-c-o1 -O1
echo "-- Building 'dima-medium-c-o1'..."
build_c dima.c dima-medium-c-o1 -DMEDIUM_TEST -O1
echo "-- Building 'dima-reserve-c-o1'..."
build_c dima.c dima-reserve-c-o1 -DDIMA_RESERVE -O1
echo "-- Building 'dima-reserve-medium-c-o1'..."
build_c dima.c dima-reserve-medium-c-o1 -DMEDIUM_TEST -DDIMA_RESERVE -O1

echo "-- Building 'malloc-c-o1'..."
build_c dima.c malloc-c-o1 -DRUN_MALLOC_TEST -O1
echo "-- Building 'malloc-medium-c-o1'..."
build_c dima.c malloc-medium-c-o1 -DRUN_MALLOC_TEST -DMEDIUM_TEST -O1

if [ "$1" != "skip_cpp" ]; then
    echo
    echo "-- Building the C++ test binaries..."

    echo "-- Building 'dima'..."
    build_cpp dima.cpp dima
    echo "-- Building 'dima-medium'..."
    build_cpp dima.cpp dima-medium -DMEDIUM_TEST
    echo "-- Building 'dima-reserve'..."
    build_cpp dima.cpp dima-reserve -DDIMA_RESERVE
    echo "-- Building 'dima-reserve-medium'..."
    build_cpp dima.cpp dima-reserve-medium -DIMA_RESERVE -DMEDIUM_TEST
    echo "-- Building 'dima-array'..."
    build_cpp dima_array.cpp dima-array
    echo "-- Building 'dima-array-medium'..."
    build_cpp dima_array.cpp dima-array-medium -DIMA_RESERVE

    echo "-- Building 'std-shared'..."
    build_cpp std_shared.cpp std-shared
    echo "-- Building 'std-shared-medium'..."
    build_cpp std_shared.cpp std-shared-medium -DMEDIUM_TEST
    echo "-- Building 'std-unique'..."
    build_cpp std_unique.cpp std-unique
    echo "-- Building 'std-unique-medium'..."
    build_cpp std_unique.cpp std-unique-medium -DMEDIUM_TEST

    echo "-- Building 'dima-o1'..."
    build_cpp dima.cpp dima-o1 -O1
    echo "-- Building 'dima-medium-o1'..."
    build_cpp dima.cpp dima-medium-o1 -DMEDIUM_TEST -O1
    echo "-- Building 'dima-reserve-o1'..."
    build_cpp dima.cpp dima-reserve-o1 -DDIMA_RESERVE -O1
    echo "-- Building 'dima-reserve-medium-o1'..."
    build_cpp dima.cpp dima-reserve-medium-o1 -DDIMA_RESERVE -DMEDIUM_TEST -O1
    echo "-- Building 'dima-array-o1'..."
    build_cpp dima_array.cpp dima-array-o1 -O1
    echo "-- Building 'dima-array-medium-o1'..."
    build_cpp dima_array.cpp dima-array-medium-o1 -DMEDIUM_TEST -O1

    echo "-- Building 'std-shared-o1'..."
    build_cpp std_shared.cpp std-shared-o1 -O1
    echo "-- Building 'std-shared-medium-o1'..."
    build_cpp std_shared.cpp std-shared-medium-o1 -DMEDIUM_TEST -O1
    echo "-- Building 'std-unique-o1'..."
    build_cpp std_unique.cpp std-unique-o1 -O1
    echo "-- Building 'std-unique-medium-o1'..."
    build_cpp std_unique.cpp std-unique-medium-o1 -DMEDIUM_TEST -O1
fi
