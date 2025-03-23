#pragma once

#include <cstddef>
#include <sys/resource.h>

size_t get_memory_usage() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss / 1024; // For MB
}
