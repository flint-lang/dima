#!/usr/bin/env sh

# $1 - file_name
# $2 - binary_name
# $3 - additional flags (for example -O1)
build() {
    clang ./test/src/"$1" -o ./out/"$2" -lstdc++ -lm -g -std=c++17 -I./ -Itest/include "$3"
}

build dima.cpp dima
build dima_medium.cpp dima-medium
build dima_reserve.cpp dima-reserve
build std_shared.cpp std-shared
build std_shared_medium.cpp std-shared-medium
build std_unique.cpp std-unique
build std_unique_medium.cpp std-unique-medium

build dima.cpp dima-o1 -O1
build dima_medium.cpp dima-medium-o1 -O1
build dima_reserve.cpp dima-reserve-o1 -O1
build std_shared.cpp std-shared-o1 -O1
build std_shared_medium.cpp std-shared-medium-o1 -O1
build std_unique.cpp std-unique-o1 -O1
build std_unique_medium.cpp std-unique-medium-o1 -O1
