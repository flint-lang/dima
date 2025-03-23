#include "formatting.hpp"
#include "memory.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <memory>
#include <vector>

class Expression {
  public:
    std::array<double, 8> values; // 64 Bytes of data

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

void apply_simple_operation(std::vector<std::shared_ptr<Expression>> &variables) {
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

void apply_complex_operation(std::vector<std::shared_ptr<Expression>> &variables) {
    for (auto &expr : variables) {
        // Operations that use more of the object data
        for (size_t i = 0; i < expr->values.size(); i++) {
            expr->values[i] = std::sin(expr->values[i]) * std::cos(expr->values[i]);
        }
    }
}

std::tuple<duration, duration, duration, size_t> test_n_allocations(const size_t n) {
    auto start = std::chrono::high_resolution_clock::now();
    auto middle = start;
    auto middle2 = start;
    size_t memory_usage;
    {
        std::vector<std::shared_ptr<Expression>> variables;
        variables.reserve(n);
        for (int i = 0; i < n; i++) {
            variables.emplace_back(std::make_shared<Expression>(std::string("expr_") + std::to_string(i)));
        }
        middle = std::chrono::high_resolution_clock::now();
        // Now the operations
        apply_simple_operation(variables);
        middle2 = std::chrono::high_resolution_clock::now();
        apply_complex_operation(variables);

        memory_usage = get_memory_usage();
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> alloc_dur = middle - start;
    std::chrono::duration<double, std::milli> calc_simp = middle2 - middle;
    std::chrono::duration<double, std::milli> calc_comp = end - middle2;
    return {alloc_dur, calc_simp, calc_comp, memory_usage};
}

int main() {
    std::vector<std::pair<size_t, std::tuple<duration, duration, duration, size_t>>> all_results;

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
    all_results.emplace_back(17000000, test_n_allocations(17000000)); // 17.000.000
    all_results.emplace_back(18000000, test_n_allocations(18000000)); // 18.000.000
    all_results.emplace_back(19000000, test_n_allocations(19000000)); // 19.000.000
    all_results.emplace_back(20000000, test_n_allocations(20000000)); // 20.000.000

    // Print formatted results
    print_results_table(all_results);
    return 0;
}
