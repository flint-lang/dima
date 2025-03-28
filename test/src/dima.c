#include "formatting.h"
#include "memory.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#define DIMA_IMPLEMENTATION
#include <dima-c/dima.h>
// #define LIKELYHOOD_TESTING

#if defined(MEDIUM_TEST)
#define VALUES_LEN 8
#else
#define VALUES_LEN 64
#endif

typedef struct {
    int x, y;
    float h;
    double values[VALUES_LEN];
} Expression;

DIMA_DEFINE(Expression, 2, 3, 0.0, {0});

/// @function `apply_simple_operation_dima`
/// @brief The simple operation is just the hypothenus of x and y
///
/// @param `variables` The variables array
/// @param `len` The length of the variables array
void apply_simple_operation_dima(Expression **variables, size_t len) {
    for (size_t i = 0; i < len; i++) {
        VAR(Expression, expr) = REF(Expression, variables[i]);
        expr->h = sqrtf(powf((float)expr->x, 2.0) + powf((float)expr->y, 2.0));
    }
}

/// @function `apply_simple_operation_malloc`
/// @brief The simple operation is just the hypothenus of x and y
///
/// @param `variables` The variables array
/// @param `len` The length of the variables array
void apply_simple_operation_malloc(Expression **variables, size_t len) {
    for (size_t i = 0; i < len; i++) {
        Expression *expr = variables[i];
        expr->h = sqrtf(powf((float)expr->x, 2.0) + powf((float)expr->y, 2.0));
    }
}

/// @function `apply_complex_operation_dima`
/// @brief The complex operation is the sin identity of all values within the Expressions struct
///
/// @param `variables` The variables array
/// @param `len` The length of the variables array
void apply_complex_operation_dima(Expression **variables, size_t len) {
    for (size_t i = 0; i < len; i++) {
        VAR(Expression, expr) = REF(Expression, variables[i]);
        double *values = expr->values;
        for (size_t j = 0; j < VALUES_LEN; j++) {
            values[j] = sin(values[j]) * cos(values[j]);
        }
    }
}

/// @function `apply_complex_operation_malloc`
/// @brief The complex operation is the sin identity of all values within the Expressions struct
///
/// @param `variables` The variables array
/// @param `len` The length of the variables array
void apply_complex_operation_malloc(Expression **variables, size_t len) {
    for (size_t i = 0; i < len; i++) {
        Expression *expr = variables[i];
        double *values = expr->values;
        for (size_t j = 0; j < VALUES_LEN; j++) {
            values[j] = sin(values[j]) * cos(values[j]);
        }
    }
}

/// @struct `Result`
/// @brief The result struct of the test
typedef struct {
    double memory_usage, alloc_time, simple_ops_time, complex_ops_time, free_time;
} Result;

/// @function `test_dima`
/// @brief Completes a test, testing n Expressions, returning the result struct
///
/// @param `n` The number of expression allocations and operations to test
/// @return `Result` The result struct containing all test results
Result test_dima(const size_t n) {
    struct timeval start, alloc_end, simple_ops_end, complex_ops_end, free_end;
    Expression **variables = malloc(sizeof(Expression *) * n);
    gettimeofday(&start, NULL);
    for (size_t i = 0; i < n; i++) {
        ALLOC(Expression, e);
        variables[i] = REF(Expression, e);
    }
    gettimeofday(&alloc_end, NULL);
    apply_simple_operation_dima(variables, n);
    gettimeofday(&simple_ops_end, NULL);
    apply_complex_operation_dima(variables, n);
    gettimeofday(&complex_ops_end, NULL);
    double memory_usage = (double)get_memory_usage();
    for (size_t i = 0; i < n; i++) {
        VAR_VALID(Expression, variables[i]);
        RELEASE(Expression, variables[i]);
    }
    free(variables);
    gettimeofday(&free_end, NULL);
    double alloc_time = (alloc_end.tv_sec - start.tv_sec) * 1000.0 + (alloc_end.tv_usec - start.tv_usec) / 1000.0;
    double simple_ops_time = (simple_ops_end.tv_sec - alloc_end.tv_sec) * 1000.0 + (simple_ops_end.tv_usec - alloc_end.tv_usec) / 1000.0;
    double complex_ops_time =
        (complex_ops_end.tv_sec - simple_ops_end.tv_sec) * 1000.0 + (complex_ops_end.tv_usec - simple_ops_end.tv_usec) / 1000.0;
    double free_time = (free_end.tv_sec - complex_ops_end.tv_sec) * 1000.0 + (free_end.tv_usec - complex_ops_end.tv_usec) / 1000.0;
    Result res = {memory_usage, alloc_time, simple_ops_time, complex_ops_time, free_time};
    return res;
}

/// @function `test_malloc`
/// @brief Completes a test, testing n Expressions, returning the result struct
///
/// @param `n` The number of expression allocations and operations to test
/// @return `Result` The result struct containing all test results
Result test_malloc(const size_t n) {
    struct timeval start, alloc_end, simple_ops_end, complex_ops_end, free_end;
    Expression **variables = malloc(sizeof(Expression *) * n);
    gettimeofday(&start, NULL);
    for (size_t i = 0; i < n; i++) {
        variables[i] = malloc(sizeof(Expression));
    }
    gettimeofday(&alloc_end, NULL);
    apply_simple_operation_malloc(variables, n);
    gettimeofday(&simple_ops_end, NULL);
    apply_complex_operation_malloc(variables, n);
    gettimeofday(&complex_ops_end, NULL);
    double memory_usage = (double)get_memory_usage();
    for (size_t i = 0; i < n; i++) {
        free(variables[i]);
    }
    free(variables);
    gettimeofday(&free_end, NULL);
    double alloc_time = (alloc_end.tv_sec - start.tv_sec) * 1000.0 + (alloc_end.tv_usec - start.tv_usec) / 1000.0;
    double simple_ops_time = (simple_ops_end.tv_sec - alloc_end.tv_sec) * 1000.0 + (simple_ops_end.tv_usec - alloc_end.tv_usec) / 1000.0;
    double complex_ops_time =
        (complex_ops_end.tv_sec - simple_ops_end.tv_sec) * 1000.0 + (complex_ops_end.tv_usec - simple_ops_end.tv_usec) / 1000.0;
    double free_time = (free_end.tv_sec - complex_ops_end.tv_sec) * 1000.0 + (free_end.tv_usec - complex_ops_end.tv_usec) / 1000.0;
    Result res = {memory_usage, alloc_time, simple_ops_time, complex_ops_time, free_time};
    return res;
}

#if defined(RUN_MALLOC_TEST)
#define RUN_TEST(N) Result result_##N = test_malloc(N);
#else
#define RUN_TEST(N) Result result_##N = test_dima(N);
#endif

#define PRINT_ROW(N)                                                                                                                       \
    print_formatted_row(N, result_##N.memory_usage, result_##N.alloc_time, result_##N.simple_ops_time, result_##N.complex_ops_time,        \
        result_##N.free_time);

int main() {
    RUN_TEST(100)
    RUN_TEST(500)
    RUN_TEST(1000)
    RUN_TEST(5000)
    RUN_TEST(10000)
    RUN_TEST(50000)
    RUN_TEST(100000)
    RUN_TEST(500000)
    RUN_TEST(1000000)
    RUN_TEST(2000000)
    RUN_TEST(3000000)
    RUN_TEST(4000000)
    RUN_TEST(5000000)
    RUN_TEST(6000000)
    RUN_TEST(7000000)
    RUN_TEST(8000000)
    RUN_TEST(9000000)
    RUN_TEST(10000000)
    RUN_TEST(11000000)
    RUN_TEST(12000000)
    RUN_TEST(13000000)
    RUN_TEST(14000000)
    RUN_TEST(15000000)
    RUN_TEST(16000000)

    print_header();
    PRINT_ROW(100)
    PRINT_ROW(500)
    PRINT_ROW(1000)
    PRINT_ROW(5000)
    PRINT_ROW(10000)
    PRINT_ROW(50000)
    PRINT_ROW(100000)
    PRINT_ROW(500000)
    PRINT_ROW(1000000)
    PRINT_ROW(2000000)
    PRINT_ROW(3000000)
    PRINT_ROW(4000000)
    PRINT_ROW(5000000)
    PRINT_ROW(6000000)
    PRINT_ROW(7000000)
    PRINT_ROW(8000000)
    PRINT_ROW(9000000)
    PRINT_ROW(10000000)
    PRINT_ROW(11000000)
    PRINT_ROW(12000000)
    PRINT_ROW(13000000)
    PRINT_ROW(14000000)
    PRINT_ROW(15000000)
    PRINT_ROW(16000000)
    print_line();

    print_likelyhoods();
    return 0;
}
