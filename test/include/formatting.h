#ifndef FORMATTING_H
#define FORMATTING_H

#include <stdio.h>

#define COL_WIDTH 15

void print_line() {
    printf("+-%-10s-+-%-*s-+-%-*s-+-%-*s-+-%-*s-+-%-*s-+\n", //
        "----------", COL_WIDTH,                             //
        "---------------", COL_WIDTH,                        //
        "---------------", COL_WIDTH,                        //
        "---------------", COL_WIDTH,                        //
        "---------------", COL_WIDTH,                        //
        "---------------"                                    //
    );
}

void print_header() {
    print_line();
    printf("| %-10s | %-*s | %-*s | %-*s | %-*s | %-*s |\n", //
        "Count", COL_WIDTH,                                  //
        "Memory Usage", COL_WIDTH,                           //
        "Allocation", COL_WIDTH,                             //
        "Simple Op", COL_WIDTH,                              //
        "Complex Op", COL_WIDTH,                             //
        "Deallocation"                                       //
    );
    print_line();
}

void print_row(const char *count, const char *usage, const char *alloc, const char *simple, const char *complex, const char *dealloc) {
    printf("| %-10s | %-*s | %-*s | %-*s | %-*s | %-*s |\n", //
        count, COL_WIDTH,                                    //
        usage, COL_WIDTH,                                    //
        alloc, COL_WIDTH,                                    //
        simple, COL_WIDTH,                                   //
        complex, COL_WIDTH,                                  //
        dealloc                                              //
    );
}

void print_formatted_row(int count, double usage_mb, double alloc_ms, double simple_ms, double complex_ms, double dealloc_ms) {
    char count_str[32], usage_str[32], alloc_str[32];
    char simple_str[32], complex_str[32], dealloc_str[32];

    sprintf(count_str, "%d", count);
    sprintf(usage_str, "%.2f MB", usage_mb);
    sprintf(alloc_str, "%.2f ms", alloc_ms);
    sprintf(simple_str, "%.2f ms", simple_ms);
    sprintf(complex_str, "%.2f ms", complex_ms);
    sprintf(dealloc_str, "%.2f ms", dealloc_ms);

    print_row(count_str, usage_str, alloc_str, simple_str, complex_str, dealloc_str);
}

#endif
