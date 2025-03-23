#!/usr/bin/env sh

clang ./test/src/dima.cpp -o ./out/dima -lstdc++ -lm -g -std=c++17 -I./ -Itest/include
clang ./test/src/dima_medium.cpp -o ./out/dima-medium -lstdc++ -lm -g -std=c++17 -I./ -Itest/include
clang ./test/src/std_shared.cpp -o ./out/std-shared -lstdc++ -lm -g -std=c++17 -I./ -Itest/include
clang ./test/src/std_shared_medium.cpp -o ./out/std-shared-medium -lstdc++ -lm -g -std=c++17 -I./ -Itest/include
clang ./test/src/std_unique.cpp -o ./out/std-unique -lstdc++ -lm -g -std=c++17 -I./ -Itest/include
clang ./test/src/std_unique_medium.cpp -o ./out/std-unique-medium -lstdc++ -lm -g -std=c++17 -I./ -Itest/include

clang ./test/src/dima.cpp -o ./out/dima-o1 -lstdc++ -lm -g -std=c++17 -I./ -Itest/include -O1
clang ./test/src/dima_medium.cpp -o ./out/dima-medium-o1 -lstdc++ -lm -g -std=c++17 -I./ -Itest/include -O1
clang ./test/src/std_shared.cpp -o ./out/std-shared-o1 -lstdc++ -lm -g -std=c++17 -I./ -Itest/include -O1
clang ./test/src/std_shared_medium.cpp -o ./out/std-shared-medium-o1 -lstdc++ -lm -g -std=c++17 -I./ -Itest/include -O1
clang ./test/src/std_unique.cpp -o ./out/std-unique-o1 -lstdc++ -lm -g -std=c++17 -I./ -Itest/include -O1
clang ./test/src/std_unique_medium.cpp -o ./out/std-unique-medium-o1 -lstdc++ -lm -g -std=c++17 -I./ -Itest/include -O1
