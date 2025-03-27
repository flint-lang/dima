#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#define DIMA_IMPLEMENTATION
#include <dima-c/dima.h>
// #define LIKELYHOOD_TESTING

#define VALUES_LEN 64
#define EXPRESSION_COUNT 100000

typedef struct {
    double values[VALUES_LEN];
} Expression;

DIMA_DEFINE(Expression, {0});

void apply_operation(Expression **variables, size_t len) {
    for (size_t i = 0; i < len; i++) {
        VAR(Expression, expr) = REF(Expression, variables[i]);
        for (size_t j = 0; j < VALUES_LEN; j++) {
            double *val = &expr->values[j];
            *val = sin(*val) * cos(*val);
        }
    }
}

int main() {
    struct timeval start, alloc_end, operations_end, end;
    Expression *variables[EXPRESSION_COUNT];
    gettimeofday(&start, NULL);
    for (size_t i = 0; i < EXPRESSION_COUNT; i++) {
        ALLOC(Expression, e);
        variables[i] = REF(Expression, e);
    }
    gettimeofday(&alloc_end, NULL);
    apply_operation(variables, EXPRESSION_COUNT);
    gettimeofday(&operations_end, NULL);
    for (size_t i = 0; i < EXPRESSION_COUNT; i++) {
        VAR_VALID(Expression, variables[i]);
        RELEASE(Expression, variables[i]);
    }
    gettimeofday(&end, NULL);

    // Print timing diagnostics
    double alloc_time = (alloc_end.tv_sec - start.tv_sec) * 1000.0 + (alloc_end.tv_usec - start.tv_usec) / 1000.0;
    double ops_time = (operations_end.tv_sec - alloc_end.tv_sec) * 1000.0 + (operations_end.tv_usec - alloc_end.tv_usec) / 1000.0;
    double free_time = (end.tv_sec - operations_end.tv_sec) * 1000.0 + (end.tv_usec - operations_end.tv_usec) / 1000.0;
    double total_time = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;

    printf("Timing diagnostics:\n");
    printf("Allocation:    %.2f ms\n", alloc_time);
    printf("Operations:    %.2f ms\n", ops_time);
    printf("Deallocations: %.2f ms\n", free_time);
    printf("Total:         %.2f ms\n\n", total_time);

    // Print likelyhood diagnostics
    LC_PRINT(0)
    LC_PRINT(1)
    LC_PRINT(2)
    LC_PRINT(3)
    LC_PRINT(4)
    LC_PRINT(5)
    LC_PRINT(6)
    LC_PRINT(7)
    LC_PRINT(8)
    LC_PRINT(9)
    LC_PRINT(10)
    LC_PRINT(11)

    return 0;
}
