#!/usr/bin/env sh

clang ./test/dima.cpp -o ./out/dima -lstdc++ -lm -g -std=c++17 -I./
clang ./test/std.cpp -o ./out/std -lstdc++ -lm -g -std=c++17 -I./

clang ./test/dima.cpp -o ./out/dima-o1 -lstdc++ -lm -g -std=c++17 -I./ -O1
clang ./test/std.cpp -o ./out/std-o1 -lstdc++ -lm -g -std=c++17 -I./ -O1
