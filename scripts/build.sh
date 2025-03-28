#!/usr/bin/env sh

# $1 - file_name
# $2 - binary_name
# $3 - additional flags (for example -O1)
build_cpp() {
    clang ./test/src/"$1" -o ./out/"$2" -lstdc++ -lm -g -std=c++17 -I./ -Itest/include "$3"
}

# $1 - file_name
# $2 - binary_name
# $3 - additional flags (for example -O1)
build_c() {
    clang ./test/src/"$1" -o ./out/"$2" -lc -lm -g -I./ -Itest/include "$3"
}

if [ "$1" = "c" ]; then
    echo "-- Building 'dima-c'..."
    build_c dima.c dima-c
    echo "-- Building 'dima-c-o1'..."
    build_c dima.c dima-c-o1 -O1
else
    echo "-- Building 'dima'..."
    build_cpp dima.cpp dima
    echo "-- Building 'dima-medium'..."
    build_cpp dima_medium.cpp dima-medium
    echo "-- Building 'dima-reserve'..."
    build_cpp dima_reserve.cpp dima-reserve
    echo "-- Building 'dima-reserve-medium'..."
    build_cpp dima_reserve_medium.cpp dima-reserve-medium

    echo "-- Building 'std-shared'..."
    build_cpp std_shared.cpp std-shared
    echo "-- Building 'std-shared-medium'..."
    build_cpp std_shared_medium.cpp std-shared-medium
    echo "-- Building 'std-unique'..."
    build_cpp std_unique.cpp std-unique
    echo "-- Building 'std-unique-medium'..."
    build_cpp std_unique_medium.cpp std-unique-medium

    echo "-- Building 'dima-o1'..."
    build_cpp dima.cpp dima-o1 -O1
    echo "-- Building 'dima-medium-o1'..."
    build_cpp dima_medium.cpp dima-medium-o1 -O1
    echo "-- Building 'dima-reserve-o1'..."
    build_cpp dima_reserve.cpp dima-reserve-o1 -O1

    echo "-- Building 'std-shared-o1'..."
    build_cpp std_shared.cpp std-shared-o1 -O1
    echo "-- Building 'std-shared-medium-o1'..."
    build_cpp std_shared_medium.cpp std-shared-medium-o1 -O1
    echo "-- Building 'std-unique-o1'..."
    build_cpp std_unique.cpp std-unique-o1 -O1
    echo "-- Building 'std-unique-medium-o1'..."
    build_cpp std_unique_medium.cpp std-unique-medium-o1 -O1
fi
