#pragma once

#include <chrono>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

// === THE PRINTING STUFF ===

using duration = std::chrono::duration<double, std::milli>;

// Function to format a duration with specific precision
std::string format_duration(const duration &d, int precision = 2) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(precision) << d.count() << "ms";
    return stream.str();
}

// Function to format a number with thousands separators
std::string format_number(size_t n) {
    std::ostringstream stream;
    stream << std::fixed;

    if (n >= 1000000) {
        stream << std::setprecision(1) << (n / 1000000.0) << "M";
    } else if (n >= 1000) {
        stream << std::setprecision(1) << (n / 1000.0) << "K";
    } else {
        stream << n;
    }

    return stream.str();
}

std::string format_memory(const size_t memory_usage) {
    std::ostringstream stream;
    stream << std::fixed << memory_usage << " MB";
    return stream.str();
}

// Print a horizontal line for the table
void print_separator(size_t width) {
    std::cout << "+" << std::string(width - 2, '-') << "+" << std::endl;
}

// Print a table row with proper formatting
void print_row(                      //
    const std::string &count,        //
    const std::string &usage,        //
    const std::string &alloc,        //
    const std::string &simple,       //
    const std::string &complex,      //
    const std::string &dealloc,      //
    const std::string &used_slots,   //
    const std::string &slot_capacity //
) {
    const int col_width = 15;
    std::cout << "| " << std::left << std::setw(10) << count    //
              << " | " << std::setw(col_width) << usage         //
              << " | " << std::setw(col_width) << alloc         //
              << " | " << std::setw(col_width) << simple        //
              << " | " << std::setw(col_width) << complex       //
              << " | " << std::setw(col_width) << dealloc       //
              << " | " << std::setw(col_width) << used_slots    //
              << " | " << std::setw(col_width) << slot_capacity //
              << " |" << std::endl;
}

// Print the results in a formatted table
void print_results_table(
    const std::vector<std::pair<size_t, std::tuple<duration, duration, duration, duration, size_t, size_t, size_t>>> &results) {
    // Table header
    std::cout << "\n=== Performance Benchmarks ===\n" << std::endl;
    print_separator(140);
    print_row("Objects", "Memory Usage", "Allocation", "Simple Ops", "Complex Ops", "Deallocation", "Used Slots", "Slot Capacity");
    print_separator(140);

    // Table content
    for (const auto &[count, timings] : results) {
        print_row(                                 //
            format_number(count),                  //
            format_memory(std::get<4>(timings)),   //
            format_duration(std::get<0>(timings)), //
            format_duration(std::get<1>(timings)), //
            format_duration(std::get<2>(timings)), //
            format_duration(std::get<3>(timings)), //
            std::to_string(std::get<5>(timings)),  //
            std::to_string(std::get<6>(timings))   //
        );
    }

    print_separator(140);
}
