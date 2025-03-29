#include "formatting.hpp"
#include "memory.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>

#include <dima/type.hpp>

class Expression : public dima::Type<Expression> {
  public:
    std::array<double, 64> values; // 512 Bytes of data

    Expression() = default;
    explicit Expression(const std::string &type) :
        type(type) {}

    void set_type(const std::string &new_type) {
        type = new_type;
    }

    std::string get_type() const {
        return type;
    }

  private:
    std::string type;
};

void apply_complex_operation(std::vector<dima::Var<Expression>> &variables) {
    for (auto &expr : variables) {
        // Operations that use more of the object data
        for (size_t i = 0; i < expr->values.size(); i++) {
            expr->values[i] = std::sin(expr->values[i]) * std::cos(expr->values[i]);
        }
    }
}

void apply_simple_operation(std::vector<dima::Var<Expression>> &variables) {
    // Use parallel_foreach to modify all expressions
    for (auto &expr : variables) {
        // Get the current type
        std::string current_type = expr->get_type();

        // Transform it
        std::transform(current_type.begin(), current_type.end(), current_type.begin(), ::toupper);

        // Update the expression
        expr->set_type(current_type + "_PROCESSED");
    }
}

std::tuple<duration, duration, duration, duration, size_t, size_t, size_t> test_n_allocations(const size_t n) {
    auto start = std::chrono::high_resolution_clock::now();
    auto alloc_time = start;
    auto simple_time = start;
    auto complex_time = start;
    auto dealloc_start = start;
    size_t memory_usage = 0;
    size_t slot_capacity = 0;
    {
        // Create multiple expressions
        std::vector<dima::Var<Expression>> variables;
        variables.reserve(n);
        for (int i = 0; i < n; i++) {
            variables.emplace_back(Expression::allocate(std::string("expr_") + std::to_string(i)));
        }
        slot_capacity = Expression::get_capacity();
        alloc_time = std::chrono::high_resolution_clock::now();

        // Now the operations
        apply_simple_operation(variables);
        simple_time = std::chrono::high_resolution_clock::now();
        apply_complex_operation(variables);
        complex_time = std::chrono::high_resolution_clock::now();

        memory_usage = get_memory_usage();
        dealloc_start = std::chrono::high_resolution_clock::now();
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> alloc_dur = alloc_time - start;
    std::chrono::duration<double, std::milli> calc_simp = simple_time - alloc_time;
    std::chrono::duration<double, std::milli> calc_comp = complex_time - simple_time;
    std::chrono::duration<double, std::milli> dealloc_time = end - dealloc_start;
    return {alloc_dur, calc_simp, calc_comp, dealloc_time, memory_usage, n, slot_capacity};
}

int main() {
    std::vector<std::pair<size_t, std::tuple<duration, duration, duration, duration, size_t, size_t, size_t>>> all_results;

    // Run tests with increasing object counts
    all_results.emplace_back(100, test_n_allocations(100));           // 100
    all_results.emplace_back(500, test_n_allocations(500));           // 500
    all_results.emplace_back(1000, test_n_allocations(1000));         // 1.000
    all_results.emplace_back(5000, test_n_allocations(5000));         // 5.000
    all_results.emplace_back(10000, test_n_allocations(10000));       // 10.000
    all_results.emplace_back(50000, test_n_allocations(50000));       // 50.000
    all_results.emplace_back(100000, test_n_allocations(100000));     // 100.000
    all_results.emplace_back(500000, test_n_allocations(500000));     // 500.000
    all_results.emplace_back(1000000, test_n_allocations(1000000));   // 1.000.000
    all_results.emplace_back(2000000, test_n_allocations(2000000));   // 2.000.000
    all_results.emplace_back(3000000, test_n_allocations(3000000));   // 3.000.000
    all_results.emplace_back(4000000, test_n_allocations(4000000));   // 4.000.000
    all_results.emplace_back(5000000, test_n_allocations(5000000));   // 5.000.000
    all_results.emplace_back(5000000, test_n_allocations(5000000));   // 5.000.000
    all_results.emplace_back(6000000, test_n_allocations(6000000));   // 6.000.000
    all_results.emplace_back(7000000, test_n_allocations(7000000));   // 7.000.000
    all_results.emplace_back(8000000, test_n_allocations(8000000));   // 8.000.000
    all_results.emplace_back(9000000, test_n_allocations(9000000));   // 9.000.000
    all_results.emplace_back(10000000, test_n_allocations(10000000)); // 10.000.000
    all_results.emplace_back(11000000, test_n_allocations(11000000)); // 11.000.000
    all_results.emplace_back(12000000, test_n_allocations(12000000)); // 12.000.000
    all_results.emplace_back(13000000, test_n_allocations(13000000)); // 13.000.000
    all_results.emplace_back(14000000, test_n_allocations(14000000)); // 14.000.000
    all_results.emplace_back(15000000, test_n_allocations(15000000)); // 15.000.000
    all_results.emplace_back(16000000, test_n_allocations(16000000)); // 16.000.000

    // Print formatted results
    print_results_table(all_results);
    return 0;
}
