#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

std::string load_file(const std::filesystem::path &path) {
    // Check if file exists
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("File does not exist: " + path.string());
    }

    // Open the file
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + path.string());
    }

    // Read the file content into a string
    std::stringstream buffer;
    buffer << file.rdbuf();

    // Return the file content as a string
    return buffer.str();
}

// A map of all columns, where the column name is the key and the column content is a map, where the row (object
// count) is a key and the value is the actual data
using parsed_file = std::unordered_map<std::string, std::unordered_map<std::string, std::string>>;

parsed_file parse_csv_file(const std::filesystem::path &file_path) {
    std::string csv_file = load_file(file_path);

    // The return map (row keys as outer keys, with inner maps of column_name -> value)
    parsed_file result;

    // Parse the CSV content
    std::istringstream stream(csv_file);
    std::string line;

    // Read the header line
    if (!std::getline(stream, line)) {
        throw std::runtime_error("CSV file is empty or header line missing");
    }

    // Parse column names from the header
    std::vector<std::string> column_names;
    std::istringstream header_stream(line);
    std::string column_name;

    while (std::getline(header_stream, column_name, ',')) {
        column_names.push_back(column_name);
    }

    // Process data rows
    while (std::getline(stream, line)) {
        std::istringstream row_stream(line);
        std::vector<std::string> row_values;
        std::string cell_value;

        // Parse all values in the row
        while (std::getline(row_stream, cell_value, ',')) {
            row_values.push_back(cell_value);
        }

        // Skip empty rows or rows with insufficient data
        if (row_values.empty() || row_values.size() < column_names.size())
            continue;

        // Get the row key (object count) from the first column
        std::string object_count = row_values[0];

        // Initialize the inner map for this row if it doesn't exist
        if (result.find(object_count) == result.end()) {
            result[object_count] = std::unordered_map<std::string, std::string>();
        }

        // For each column, store the value mapped to the column name
        for (size_t i = 0; i < column_names.size() && i < row_values.size(); ++i) {
            result[object_count][column_names[i]] = row_values[i];
        }
    }

    return result;
}

void save_parsed_file_at(const parsed_file &file, const std::filesystem::path &path) {
    // Create directories if they don't exist
    std::filesystem::create_directories(path.parent_path());

    // Open the output file
    std::ofstream out_file(path);
    if (!out_file.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + path.string());
    }

    // Collect all unique column names from all rows
    std::unordered_set<std::string> column_name_set;
    for (const auto &[_, row_data] : file) {
        for (const auto &[col_name, _] : row_data) {
            column_name_set.insert(col_name);
        }
    }

    // Convert to vector for ordering
    std::vector<std::string> column_names(column_name_set.begin(), column_name_set.end());

    // Ensure "Objects" is the first column if it exists
    auto objects_iter = std::find(column_names.begin(), column_names.end(), "Objects");
    if (objects_iter != column_names.end()) {
        std::rotate(column_names.begin(), objects_iter, objects_iter + 1);
    }

    // Helper function to convert abbreviated counts to full numeric values
    auto parse_object_count = [](const std::string &s) -> std::pair<double, std::string> {
        std::string num_part;
        std::string unit_part;

        for (char c : s) {
            if (std::isdigit(c) || c == '.') {
                num_part += c;
            } else {
                unit_part += c;
            }
        }

        double value = num_part.empty() ? 0.0 : std::stod(num_part);

        // Convert unit to multiplier
        double multiplier = 1.0;
        if (unit_part == "K")
            multiplier = 1e3;
        else if (unit_part == "M")
            multiplier = 1e6;
        else if (unit_part == "G")
            multiplier = 1e9;

        return {value * multiplier, unit_part};
    };

    // Collect all row keys (object counts) and their numeric values
    std::vector<std::pair<std::string, double>> row_keys_with_values;
    for (const auto &[object_count, unused1] : file) {
        auto [numeric_value, unused2] = parse_object_count(object_count);
        row_keys_with_values.emplace_back(object_count, numeric_value);
    }

    // Sort by numeric value
    std::sort(row_keys_with_values.begin(), row_keys_with_values.end(), [](const auto &a, const auto &b) { return a.second < b.second; });

    // Write the header row (column names)
    for (size_t i = 0; i < column_names.size(); ++i) {
        out_file << column_names[i];
        if (i < column_names.size() - 1) {
            out_file << ",";
        }
    }
    out_file << "\n";

    // Write each data row
    for (const auto &[key, numeric_value] : row_keys_with_values) {
        const auto &row_data = file.at(key);

        for (size_t i = 0; i < column_names.size(); ++i) {
            const std::string &col_name = column_names[i];

            // For the "Objects" column, use the numeric value rather than the abbreviated form
            if (col_name == "Objects") {
                // Output the full number without abbreviation
                out_file << static_cast<long long>(numeric_value);
            }
            // For other columns, use the stored value
            else if (row_data.find(col_name) != row_data.end()) {
                out_file << row_data.at(col_name);
            }

            if (i < column_names.size() - 1) {
                out_file << ",";
            }
        }
        out_file << "\n";
    }

    out_file.close();

    if (!out_file) {
        throw std::runtime_error("Failed to write to file: " + path.string());
    }
}

static std::filesystem::path results_path;

// === THE C++ CSV FILES ===
static parsed_file dima_csv;
static parsed_file dima_o1_csv;
static parsed_file dima_medium_csv;
static parsed_file dima_medium_o1_csv;

static parsed_file dima_reserve_csv;
static parsed_file dima_reserve_o1_csv;
static parsed_file dima_reserve_medium_csv;
static parsed_file dima_reserve_medium_o1_csv;

static parsed_file dima_array_csv;
static parsed_file dima_array_o1_csv;
static parsed_file dima_array_medium_csv;
static parsed_file dima_array_medium_o1_csv;

static parsed_file std_shared_csv;
static parsed_file std_shared_o1_csv;
static parsed_file std_shared_medium_csv;
static parsed_file std_shared_medium_o1_csv;

static parsed_file std_unique_csv;
static parsed_file std_unique_o1_csv;
static parsed_file std_unique_medium_csv;
static parsed_file std_unique_medium_o1_csv;

// === THE C CSV FILES ===
static parsed_file dima_c_csv;
static parsed_file dima_c_o1_csv;
static parsed_file dima_medium_c_csv;
static parsed_file dima_medium_c_o1_csv;

static parsed_file dima_reserve_c_csv;
static parsed_file dima_reserve_c_o1_csv;
static parsed_file dima_reserve_medium_c_csv;
static parsed_file dima_reserve_medium_c_o1_csv;

static parsed_file malloc_c_csv;
static parsed_file malloc_c_o1_csv;
static parsed_file malloc_medium_c_csv;
static parsed_file malloc_medium_c_o1_csv;

void load_parsed_files() {
    // === THE C++ CSV FILES ===
    dima_csv = parse_csv_file(results_path / "test_data" / "csv" / "cpp" / "dima.csv");
    dima_o1_csv = parse_csv_file(results_path / "test_data" / "csv" / "cpp" / "dima-o1.csv");
    dima_medium_csv = parse_csv_file(results_path / "test_data" / "csv" / "cpp" / "dima-medium.csv");
    dima_medium_o1_csv = parse_csv_file(results_path / "test_data" / "csv" / "cpp" / "dima-medium-o1.csv");

    dima_reserve_csv = parse_csv_file(results_path / "test_data" / "csv" / "cpp" / "dima-reserve.csv");
    dima_reserve_o1_csv = parse_csv_file(results_path / "test_data" / "csv" / "cpp" / "dima-reserve-o1.csv");
    dima_reserve_medium_csv = parse_csv_file(results_path / "test_data" / "csv" / "cpp" / "dima-reserve-medium.csv");
    dima_reserve_medium_o1_csv = parse_csv_file(results_path / "test_data" / "csv" / "cpp" / "dima-reserve-medium-o1.csv");

    dima_array_csv = parse_csv_file(results_path / "test_data" / "csv" / "cpp" / "dima-array.csv");
    dima_array_o1_csv = parse_csv_file(results_path / "test_data" / "csv" / "cpp" / "dima-array-o1.csv");
    dima_array_medium_csv = parse_csv_file(results_path / "test_data" / "csv" / "cpp" / "dima-array-medium.csv");
    dima_array_medium_o1_csv = parse_csv_file(results_path / "test_data" / "csv" / "cpp" / "dima-array-medium-o1.csv");

    std_shared_csv = parse_csv_file(results_path / "test_data" / "csv" / "cpp" / "std-shared.csv");
    std_shared_o1_csv = parse_csv_file(results_path / "test_data" / "csv" / "cpp" / "std-shared-o1.csv");
    std_shared_medium_csv = parse_csv_file(results_path / "test_data" / "csv" / "cpp" / "std-shared-medium.csv");
    std_shared_medium_o1_csv = parse_csv_file(results_path / "test_data" / "csv" / "cpp" / "std-shared-medium-o1.csv");

    std_unique_csv = parse_csv_file(results_path / "test_data" / "csv" / "cpp" / "std-unique.csv");
    std_unique_o1_csv = parse_csv_file(results_path / "test_data" / "csv" / "cpp" / "std-unique-o1.csv");
    std_unique_medium_csv = parse_csv_file(results_path / "test_data" / "csv" / "cpp" / "std-unique-medium.csv");
    std_unique_medium_o1_csv = parse_csv_file(results_path / "test_data" / "csv" / "cpp" / "std-unique-medium-o1.csv");

    // === THE C CSV FILES ===
    dima_c_csv = parse_csv_file(results_path / "test_data" / "csv" / "c" / "dima-c.csv");
    dima_c_o1_csv = parse_csv_file(results_path / "test_data" / "csv" / "c" / "dima-c-o1.csv");
    dima_medium_c_csv = parse_csv_file(results_path / "test_data" / "csv" / "c" / "dima-medium-c.csv");
    dima_medium_c_o1_csv = parse_csv_file(results_path / "test_data" / "csv" / "c" / "dima-medium-c-o1.csv");

    dima_reserve_c_csv = parse_csv_file(results_path / "test_data" / "csv" / "c" / "dima-reserve-c.csv");
    dima_reserve_c_o1_csv = parse_csv_file(results_path / "test_data" / "csv" / "c" / "dima-reserve-c-o1.csv");
    dima_reserve_medium_c_csv = parse_csv_file(results_path / "test_data" / "csv" / "c" / "dima-reserve-medium-c.csv");
    dima_reserve_medium_c_o1_csv = parse_csv_file(results_path / "test_data" / "csv" / "c" / "dima-reserve-medium-c-o1.csv");

    malloc_c_csv = parse_csv_file(results_path / "test_data" / "csv" / "c" / "malloc-c.csv");
    malloc_c_o1_csv = parse_csv_file(results_path / "test_data" / "csv" / "c" / "malloc-c-o1.csv");
    malloc_medium_c_csv = parse_csv_file(results_path / "test_data" / "csv" / "c" / "malloc-medium-c.csv");
    malloc_medium_c_o1_csv = parse_csv_file(results_path / "test_data" / "csv" / "c" / "malloc-medium-c-o1.csv");
}

/**
 * Extracts a column from the source parsed file and adds it to the target parsed file under a new name.
 *
 * @param target The target parsed file to add the column to
 * @param source The source parsed file to extract the column from
 * @param source_column_name The name of the column to extract from the source
 * @param target_column_name The name to give the column in the target
 */
void extract_column_and_add_to(            //
    parsed_file &target,                   //
    const parsed_file &source,             //
    const std::string &source_column_name, //
    const std::string &target_column_name  //
) {
    // Iterate through all rows (object counts) in the source
    for (const auto &[object_count, row_data] : source) {
        // Check if this row has the source column
        if (row_data.find(source_column_name) != row_data.end()) {
            // Get the value from the source column
            const std::string &value = row_data.at(source_column_name);

            // If this object count doesn't exist in the target yet, create it
            if (target.find(object_count) == target.end()) {
                target[object_count] = std::unordered_map<std::string, std::string>();
            }

            // Add the value to the target with the new column name
            target[object_count][target_column_name] = value;
        }
    }
}

void create_csv_memory_usage() {
    // The memory usage takes all columns from all non-medium tests and collects their respective memory usage
    // The memory usage is exactly the same in -o1 and non optimized code, so we can leave out all optimized versions entirely
    parsed_file extracted_data;
    extract_column_and_add_to(extracted_data, dima_csv, "Objects", "Objects");
    extract_column_and_add_to(extracted_data, dima_csv, "Memory Usage", "Dima");
    extract_column_and_add_to(extracted_data, dima_reserve_csv, "Memory Usage", "Dima-Reserve");
    extract_column_and_add_to(extracted_data, dima_array_csv, "Memory Usage", "Dima-Array");
    extract_column_and_add_to(extracted_data, dima_c_csv, "Memory Usage", "Dima-C");
    extract_column_and_add_to(extracted_data, dima_reserve_c_csv, "Memory Usage", "Dima-Reserve-C");
    extract_column_and_add_to(extracted_data, std_shared_csv, "Memory Usage", "Shared");
    extract_column_and_add_to(extracted_data, std_unique_csv, "Memory Usage", "Unique");
    extract_column_and_add_to(extracted_data, malloc_c_csv, "Memory Usage", "Malloc-C");
    save_parsed_file_at(extracted_data, results_path / "test_data" / "processed" / "memory-usage.csv");
}

void create_csv_memory_usage_medium() {
    parsed_file extracted_data;
    extract_column_and_add_to(extracted_data, dima_medium_csv, "Objects", "Objects");
    extract_column_and_add_to(extracted_data, dima_medium_csv, "Memory Usage", "Dima");
    extract_column_and_add_to(extracted_data, dima_reserve_medium_csv, "Memory Usage", "Dima-Reserve");
    extract_column_and_add_to(extracted_data, dima_array_medium_csv, "Memory Usage", "Dima-Array");
    extract_column_and_add_to(extracted_data, dima_medium_c_csv, "Memory Usage", "Dima-C");
    extract_column_and_add_to(extracted_data, dima_reserve_medium_c_csv, "Memory Usage", "Dima-Reserve-C");
    extract_column_and_add_to(extracted_data, std_shared_medium_csv, "Memory Usage", "Shared");
    extract_column_and_add_to(extracted_data, std_unique_medium_csv, "Memory Usage", "Unique");
    extract_column_and_add_to(extracted_data, malloc_medium_c_csv, "Memory Usage", "Malloc-C");
    save_parsed_file_at(extracted_data, results_path / "test_data" / "processed" / "memory-usage-medium.csv");
}

void create_csv_alloc_time() {
    parsed_file extracted_data;
    extract_column_and_add_to(extracted_data, dima_csv, "Objects", "Objects");
    extract_column_and_add_to(extracted_data, dima_o1_csv, "Allocation", "Dima-O1");
    extract_column_and_add_to(extracted_data, dima_reserve_csv, "Allocation", "Dima-Reserve");
    extract_column_and_add_to(extracted_data, dima_reserve_o1_csv, "Allocation", "Dima-Reserve-O1");
    extract_column_and_add_to(extracted_data, dima_array_csv, "Allocation", "Dima-Array");
    extract_column_and_add_to(extracted_data, dima_array_o1_csv, "Allocation", "Dima-Array-O1");
    extract_column_and_add_to(extracted_data, std_shared_csv, "Allocation", "Shared");
    extract_column_and_add_to(extracted_data, std_shared_o1_csv, "Allocation", "Shared-O1");
    extract_column_and_add_to(extracted_data, std_unique_csv, "Allocation", "Unique");
    extract_column_and_add_to(extracted_data, std_unique_o1_csv, "Allocation", "Unique-O1");
    extract_column_and_add_to(extracted_data, dima_csv, "Allocation", "Dima");
    save_parsed_file_at(extracted_data, results_path / "test_data" / "processed" / "alloc.csv");
}

void create_csv_alloc_time_c() {
    parsed_file extracted_data;
    extract_column_and_add_to(extracted_data, dima_c_csv, "Objects", "Objects");
    extract_column_and_add_to(extracted_data, dima_c_o1_csv, "Allocation", "Dima-O1");
    extract_column_and_add_to(extracted_data, dima_reserve_c_csv, "Allocation", "Dima-Reserve");
    extract_column_and_add_to(extracted_data, dima_reserve_c_o1_csv, "Allocation", "Dima-Reserve-O1");
    extract_column_and_add_to(extracted_data, malloc_c_csv, "Allocation", "Malloc");
    extract_column_and_add_to(extracted_data, malloc_c_o1_csv, "Allocation", "Malloc-C");
    extract_column_and_add_to(extracted_data, dima_c_csv, "Allocation", "Dima");
    save_parsed_file_at(extracted_data, results_path / "test_data" / "processed" / "alloc-c.csv");
}

void create_csv_alloc_time_medium() {
    parsed_file extracted_data;
    extract_column_and_add_to(extracted_data, dima_medium_csv, "Objects", "Objects");
    extract_column_and_add_to(extracted_data, dima_medium_o1_csv, "Allocation", "Dima-O1");
    extract_column_and_add_to(extracted_data, dima_reserve_medium_csv, "Allocation", "Dima-Reserve");
    extract_column_and_add_to(extracted_data, dima_reserve_medium_o1_csv, "Allocation", "Dima-Reserve-O1");
    extract_column_and_add_to(extracted_data, dima_array_medium_csv, "Allocation", "Dima-Array");
    extract_column_and_add_to(extracted_data, dima_array_medium_o1_csv, "Allocation", "Dima-Array-O1");
    extract_column_and_add_to(extracted_data, std_shared_medium_csv, "Allocation", "Shared");
    extract_column_and_add_to(extracted_data, std_shared_medium_o1_csv, "Allocation", "Shared-O1");
    extract_column_and_add_to(extracted_data, std_unique_medium_csv, "Allocation", "Unique");
    extract_column_and_add_to(extracted_data, std_unique_medium_o1_csv, "Allocation", "Unique-O1");
    extract_column_and_add_to(extracted_data, dima_medium_csv, "Allocation", "Dima");
    save_parsed_file_at(extracted_data, results_path / "test_data" / "processed" / "alloc-medium.csv");
}

void create_csv_alloc_time_medium_c() {
    parsed_file extracted_data;
    extract_column_and_add_to(extracted_data, dima_medium_c_csv, "Objects", "Objects");
    extract_column_and_add_to(extracted_data, dima_medium_c_o1_csv, "Allocation", "Dima-O1");
    extract_column_and_add_to(extracted_data, dima_reserve_medium_c_csv, "Allocation", "Dima-Reserve");
    extract_column_and_add_to(extracted_data, dima_reserve_medium_c_o1_csv, "Allocation", "Dima-Reserve-O1");
    extract_column_and_add_to(extracted_data, malloc_medium_c_csv, "Allocation", "Malloc");
    extract_column_and_add_to(extracted_data, malloc_medium_c_o1_csv, "Allocation", "Malloc-O1");
    extract_column_and_add_to(extracted_data, dima_medium_c_csv, "Allocation", "Dima");
    save_parsed_file_at(extracted_data, results_path / "test_data" / "processed" / "alloc-medium-c.csv");
}

void create_csv_dealloc_time() {
    parsed_file extracted_data;
    extract_column_and_add_to(extracted_data, dima_csv, "Objects", "Objects");
    extract_column_and_add_to(extracted_data, dima_o1_csv, "Deallocation", "Dima-O1");
    extract_column_and_add_to(extracted_data, dima_reserve_csv, "Deallocation", "Dima-Reserve");
    extract_column_and_add_to(extracted_data, dima_reserve_o1_csv, "Deallocation", "Dima-Reserve-O1");
    extract_column_and_add_to(extracted_data, dima_array_csv, "Deallocation", "Dima-Array");
    extract_column_and_add_to(extracted_data, dima_array_o1_csv, "Deallocation", "Dima-Array-O1");
    extract_column_and_add_to(extracted_data, std_shared_csv, "Deallocation", "Shared");
    extract_column_and_add_to(extracted_data, std_shared_o1_csv, "Deallocation", "Shared-O1");
    extract_column_and_add_to(extracted_data, std_unique_csv, "Deallocation", "Unique");
    extract_column_and_add_to(extracted_data, std_unique_o1_csv, "Deallocation", "Unique-O1");
    extract_column_and_add_to(extracted_data, dima_csv, "Deallocation", "Dima");
    save_parsed_file_at(extracted_data, results_path / "test_data" / "processed" / "dealloc.csv");
}

void create_csv_dealloc_time_c() {
    parsed_file extracted_data;
    extract_column_and_add_to(extracted_data, dima_c_csv, "Objects", "Objects");
    extract_column_and_add_to(extracted_data, dima_c_o1_csv, "Deallocation", "Dima-O1");
    extract_column_and_add_to(extracted_data, dima_reserve_c_csv, "Deallocation", "Dima-Reserve");
    extract_column_and_add_to(extracted_data, dima_reserve_c_o1_csv, "Deallocation", "Dima-Reserve-O1");
    extract_column_and_add_to(extracted_data, malloc_c_csv, "Deallocation", "Malloc");
    extract_column_and_add_to(extracted_data, malloc_c_o1_csv, "Deallocation", "Malloc-O1");
    extract_column_and_add_to(extracted_data, dima_c_csv, "Deallocation", "Dima");
    save_parsed_file_at(extracted_data, results_path / "test_data" / "processed" / "dealloc-c.csv");
}

void create_csv_dealloc_time_medium() {
    parsed_file extracted_data;
    extract_column_and_add_to(extracted_data, dima_medium_csv, "Objects", "Objects");
    extract_column_and_add_to(extracted_data, dima_medium_o1_csv, "Deallocation", "Dima-O1");
    extract_column_and_add_to(extracted_data, dima_reserve_medium_csv, "Deallocation", "Dima-Reserve");
    extract_column_and_add_to(extracted_data, dima_reserve_medium_o1_csv, "Deallocation", "Dima-Reserve-O1");
    extract_column_and_add_to(extracted_data, dima_array_medium_csv, "Deallocation", "Dima-Array");
    extract_column_and_add_to(extracted_data, dima_array_medium_o1_csv, "Deallocation", "Dima-Array-O1");
    extract_column_and_add_to(extracted_data, std_shared_medium_csv, "Deallocation", "Shared");
    extract_column_and_add_to(extracted_data, std_shared_medium_o1_csv, "Deallocation", "Shared-O1");
    extract_column_and_add_to(extracted_data, std_unique_medium_csv, "Deallocation", "Unique");
    extract_column_and_add_to(extracted_data, std_unique_medium_o1_csv, "Deallocation", "Unique-O1");
    extract_column_and_add_to(extracted_data, dima_medium_csv, "Deallocation", "Dima");
    save_parsed_file_at(extracted_data, results_path / "test_data" / "processed" / "dealloc-medium.csv");
}

void create_csv_dealloc_time_medium_c() {
    parsed_file extracted_data;
    extract_column_and_add_to(extracted_data, dima_medium_c_csv, "Objects", "Objects");
    extract_column_and_add_to(extracted_data, dima_medium_c_o1_csv, "Deallocation", "Dima-O1");
    extract_column_and_add_to(extracted_data, dima_reserve_medium_c_csv, "Deallocation", "Dima-Reserve");
    extract_column_and_add_to(extracted_data, dima_reserve_medium_c_o1_csv, "Deallocation", "Dima-Reserve-O1");
    extract_column_and_add_to(extracted_data, malloc_medium_c_csv, "Deallocation", "Malloc");
    extract_column_and_add_to(extracted_data, malloc_medium_c_o1_csv, "Deallocation", "Malloc-O1");
    extract_column_and_add_to(extracted_data, dima_medium_c_csv, "Deallocation", "Dima");
    save_parsed_file_at(extracted_data, results_path / "test_data" / "processed" / "dealloc-medium-c.csv");
}

void create_csv_simple_op() {
    parsed_file extracted_data;
    extract_column_and_add_to(extracted_data, dima_csv, "Objects", "Objects");
    extract_column_and_add_to(extracted_data, dima_o1_csv, "Simple Ops", "Dima-O1");
    extract_column_and_add_to(extracted_data, dima_reserve_csv, "Simple Ops", "Dima-Reserve");
    extract_column_and_add_to(extracted_data, dima_reserve_o1_csv, "Simple Ops", "Dima-Reserve-O1");
    extract_column_and_add_to(extracted_data, dima_array_csv, "Simple Ops", "Dima-Array");
    extract_column_and_add_to(extracted_data, dima_array_o1_csv, "Simple Ops", "Dima-Array-O1");
    extract_column_and_add_to(extracted_data, std_shared_csv, "Simple Ops", "Shared");
    extract_column_and_add_to(extracted_data, std_shared_o1_csv, "Simple Ops", "Shared-O1");
    extract_column_and_add_to(extracted_data, std_unique_csv, "Simple Ops", "Unique");
    extract_column_and_add_to(extracted_data, std_unique_o1_csv, "Simple Ops", "Unique-O1");
    extract_column_and_add_to(extracted_data, dima_csv, "Simple Ops", "Dima");
    save_parsed_file_at(extracted_data, results_path / "test_data" / "processed" / "simple-ops.csv");
}

void create_csv_simple_op_c() {
    parsed_file extracted_data;
    extract_column_and_add_to(extracted_data, dima_c_csv, "Objects", "Objects");
    extract_column_and_add_to(extracted_data, dima_c_o1_csv, "Simple Ops", "Dima-O1");
    extract_column_and_add_to(extracted_data, dima_reserve_c_csv, "Simple Ops", "Dima-Reserve");
    extract_column_and_add_to(extracted_data, dima_reserve_c_o1_csv, "Simple Ops", "Dima-Reserve-O1");
    extract_column_and_add_to(extracted_data, malloc_c_csv, "Simple Ops", "Malloc");
    extract_column_and_add_to(extracted_data, malloc_c_o1_csv, "Simple Ops", "Malloc-O1");
    extract_column_and_add_to(extracted_data, dima_c_csv, "Simple Ops", "Dima");
    save_parsed_file_at(extracted_data, results_path / "test_data" / "processed" / "simple-ops-c.csv");
}

void create_csv_simple_op_medium() {
    parsed_file extracted_data;
    extract_column_and_add_to(extracted_data, dima_medium_csv, "Objects", "Objects");
    extract_column_and_add_to(extracted_data, dima_medium_o1_csv, "Simple Ops", "Dima-O1");
    extract_column_and_add_to(extracted_data, dima_reserve_medium_csv, "Simple Ops", "Dima-Reserve");
    extract_column_and_add_to(extracted_data, dima_reserve_medium_o1_csv, "Simple Ops", "Dima-Reserve-O1");
    extract_column_and_add_to(extracted_data, dima_array_medium_csv, "Simple Ops", "Dima-Array");
    extract_column_and_add_to(extracted_data, dima_array_medium_o1_csv, "Simple Ops", "Dima-Array-O1");
    extract_column_and_add_to(extracted_data, std_shared_medium_csv, "Simple Ops", "Shared");
    extract_column_and_add_to(extracted_data, std_shared_medium_o1_csv, "Simple Ops", "Shared-O1");
    extract_column_and_add_to(extracted_data, std_unique_medium_csv, "Simple Ops", "Unique");
    extract_column_and_add_to(extracted_data, std_unique_medium_o1_csv, "Simple Ops", "Unique-O1");
    extract_column_and_add_to(extracted_data, dima_medium_csv, "Simple Ops", "Dima");
    save_parsed_file_at(extracted_data, results_path / "test_data" / "processed" / "simple-ops-medium.csv");
}

void create_csv_simple_op_medium_c() {
    parsed_file extracted_data;
    extract_column_and_add_to(extracted_data, dima_medium_c_csv, "Objects", "Objects");
    extract_column_and_add_to(extracted_data, dima_medium_c_o1_csv, "Simple Ops", "Dima-O1");
    extract_column_and_add_to(extracted_data, dima_reserve_medium_c_csv, "Simple Ops", "Dima-Reserve");
    extract_column_and_add_to(extracted_data, dima_reserve_medium_c_o1_csv, "Simple Ops", "Dima-Reserve-O1");
    extract_column_and_add_to(extracted_data, malloc_medium_c_csv, "Simple Ops", "Malloc");
    extract_column_and_add_to(extracted_data, malloc_medium_c_o1_csv, "Simple Ops", "Malloc-O1");
    extract_column_and_add_to(extracted_data, dima_medium_c_csv, "Simple Ops", "Dima");
    save_parsed_file_at(extracted_data, results_path / "test_data" / "processed" / "simple-ops-medium-c.csv");
}

void create_csv_complex_op() {
    parsed_file extracted_data;
    extract_column_and_add_to(extracted_data, dima_csv, "Objects", "Objects");
    extract_column_and_add_to(extracted_data, dima_o1_csv, "Complex Ops", "Dima-O1");
    extract_column_and_add_to(extracted_data, dima_reserve_csv, "Complex Ops", "Dima-Reserve");
    extract_column_and_add_to(extracted_data, dima_reserve_o1_csv, "Complex Ops", "Dima-Reserve-O1");
    extract_column_and_add_to(extracted_data, dima_array_csv, "Complex Ops", "Dima-Array");
    extract_column_and_add_to(extracted_data, dima_array_o1_csv, "Complex Ops", "Dima-Array-O1");
    extract_column_and_add_to(extracted_data, std_shared_csv, "Complex Ops", "Shared");
    extract_column_and_add_to(extracted_data, std_shared_o1_csv, "Complex Ops", "Shared-O1");
    extract_column_and_add_to(extracted_data, std_unique_csv, "Complex Ops", "Unique");
    extract_column_and_add_to(extracted_data, std_unique_o1_csv, "Complex Ops", "Unique-O1");
    extract_column_and_add_to(extracted_data, dima_csv, "Complex Ops", "Dima");
    save_parsed_file_at(extracted_data, results_path / "test_data" / "processed" / "complex-ops.csv");
}

void create_csv_complex_op_c() {
    parsed_file extracted_data;
    extract_column_and_add_to(extracted_data, dima_c_csv, "Objects", "Objects");
    extract_column_and_add_to(extracted_data, dima_c_o1_csv, "Complex Ops", "Dima-O1");
    extract_column_and_add_to(extracted_data, dima_reserve_c_csv, "Complex Ops", "Dima-Reserve");
    extract_column_and_add_to(extracted_data, dima_reserve_c_o1_csv, "Complex Ops", "Dima-Reserve-O1");
    extract_column_and_add_to(extracted_data, malloc_c_csv, "Complex Ops", "Malloc");
    extract_column_and_add_to(extracted_data, malloc_c_o1_csv, "Complex Ops", "Malloc-O1");
    extract_column_and_add_to(extracted_data, dima_c_csv, "Complex Ops", "Dima");
    save_parsed_file_at(extracted_data, results_path / "test_data" / "processed" / "complex-ops-c.csv");
}

void create_csv_complex_op_medium() {
    parsed_file extracted_data;
    extract_column_and_add_to(extracted_data, dima_medium_csv, "Objects", "Objects");
    extract_column_and_add_to(extracted_data, dima_medium_o1_csv, "Complex Ops", "Dima-O1");
    extract_column_and_add_to(extracted_data, dima_reserve_medium_csv, "Complex Ops", "Dima-Reserve");
    extract_column_and_add_to(extracted_data, dima_reserve_medium_o1_csv, "Complex Ops", "Dima-Reserve-O1");
    extract_column_and_add_to(extracted_data, dima_array_medium_csv, "Complex Ops", "Dima-Array");
    extract_column_and_add_to(extracted_data, dima_array_medium_o1_csv, "Complex Ops", "Dima-Array-O1");
    extract_column_and_add_to(extracted_data, std_shared_medium_csv, "Complex Ops", "Shared");
    extract_column_and_add_to(extracted_data, std_shared_medium_o1_csv, "Complex Ops", "Shared-O1");
    extract_column_and_add_to(extracted_data, std_unique_medium_csv, "Complex Ops", "Unique");
    extract_column_and_add_to(extracted_data, std_unique_medium_o1_csv, "Complex Ops", "Unique-O1");
    extract_column_and_add_to(extracted_data, dima_medium_csv, "Complex Ops", "Dima");
    save_parsed_file_at(extracted_data, results_path / "test_data" / "processed" / "complex-ops-medium.csv");
}

void create_csv_complex_op_medium_c() {
    parsed_file extracted_data;
    extract_column_and_add_to(extracted_data, dima_medium_c_csv, "Objects", "Objects");
    extract_column_and_add_to(extracted_data, dima_medium_c_o1_csv, "Complex Ops", "Dima-O1");
    extract_column_and_add_to(extracted_data, dima_reserve_medium_c_csv, "Complex Ops", "Dima-Reserve");
    extract_column_and_add_to(extracted_data, dima_reserve_medium_c_o1_csv, "Complex Ops", "Dima-Reserve-O1");
    extract_column_and_add_to(extracted_data, malloc_medium_c_csv, "Complex Ops", "Malloc");
    extract_column_and_add_to(extracted_data, malloc_medium_c_o1_csv, "Complex Ops", "Malloc-O1");
    extract_column_and_add_to(extracted_data, dima_medium_c_csv, "Complex Ops", "Dima");
    save_parsed_file_at(extracted_data, results_path / "test_data" / "processed" / "complex-ops-medium-c.csv");
}

// This is a dirty little script to create our graphs from the given csv files
int main(int argc, char *argv[]) {
    // The first argument should be the given graph data that should be collected
    if (argc != 2) {
        std::cout << "The wrong number of arguments were provided!" << std::endl;
        return 1;
    }
    results_path = std::filesystem::path(std::string(argv[0])).parent_path() / "results";
    load_parsed_files();
    const std::string arg(argv[1]);
    if (arg == "memory-usage") {
        create_csv_memory_usage();
    } else if (arg == "memory-usage-medium") {
        create_csv_memory_usage_medium();
    } else if (arg == "alloc-time") {
        create_csv_alloc_time();
    } else if (arg == "alloc-time-c") {
        create_csv_alloc_time_c();
    } else if (arg == "alloc-time-medium") {
        create_csv_alloc_time_medium();
    } else if (arg == "alloc-time-medium-c") {
        create_csv_alloc_time_medium_c();
    } else if (arg == "dealloc-time") {
        create_csv_dealloc_time();
    } else if (arg == "dealloc-time-c") {
        create_csv_dealloc_time_c();
    } else if (arg == "dealloc-time-medium") {
        create_csv_dealloc_time_medium();
    } else if (arg == "dealloc-time-medium-c") {
        create_csv_dealloc_time_medium_c();
    } else if (arg == "simple-op") {
        create_csv_simple_op();
    } else if (arg == "simple-op-c") {
        create_csv_simple_op_c();
    } else if (arg == "simple-op-medium") {
        create_csv_simple_op_medium();
    } else if (arg == "simple-op-medium-c") {
        create_csv_simple_op_medium_c();
    } else if (arg == "complex-op") {
        create_csv_complex_op();
    } else if (arg == "complex-op-c") {
        create_csv_complex_op_c();
    } else if (arg == "complex-op-medium") {
        create_csv_complex_op_medium();
    } else if (arg == "complex-op-medium-c") {
        create_csv_complex_op_medium_c();
    } else {
        std::cout << "unknown argument: " << arg << std::endl;
        return 1;
    }
    return 0;
}
