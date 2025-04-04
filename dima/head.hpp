#pragma once

#include "block.hpp"
#include "var.hpp"

#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>
#include <vector>

/// @namespace `dima`
/// @brief The `dima` namespace contains all classes used for the DIMA memory management system
namespace dima {

    /// @class `Head`
    /// @brief The head structure managing all allocated blocks, with incremental growth
    template <typename T, typename = std::enable_if_t<std::is_class_v<T>>> //
    class Head {
      public:
        /// @function `allocate`
        /// @brief Creates a new variable of type `T` and saves it in one of the blocks
        ///
        /// @param `args` The arguments with which to create the type T slot
        /// @return `Var<T>` A variable node to the allocated object of type `T`
        template <typename... Args> Var<T> allocate(Args &&...args) {
            // Try to allocate in an existing block
            for (size_t i = blocks.size(); i > 0; i--) {
                auto *block_ptr = blocks[i - 1].get();
                if (block_ptr == nullptr) {
                    continue;
                }
                if (block_ptr->get_free_count() > 0) {
                    auto var = block_ptr->allocate(std::forward<Args>(args)...);
                    if (var.has_value()) {
                        return var.value();
                    }
                }
            }
            // Apply the block mutex, as now definitely a new block will be added one way or the other
            std::lock_guard<std::mutex> lock(blocks_mutex);

            // Try to cerate a block that isnt created yet in the current blocks vector
            for (size_t i = blocks.size(); i > 0; i--) {
                if (blocks[i - 1] != nullptr) {
                    continue;
                }
                blocks[i - 1] = std::make_unique<Block<T>>(i - 1, BASE_SIZE << (i - 1));
                blocks[i - 1]->set_empty_callback([this](Block<T> *empty_block) { this->block_emptied(empty_block); });
                return blocks[i - 1]->allocate(std::forward<Args>(args)...).value();
            }

            // If all blocks are full, create a new block with 2x size of the last one, a new block definitely has space for a new variable
            const size_t block_id = blocks.size();
            const size_t new_size = blocks.empty() ? BASE_SIZE : (BASE_SIZE << block_id);
            blocks.emplace_back(std::make_unique<Block<T>>(block_id, new_size));
            blocks.back()->set_empty_callback([this](Block<T> *empty_block) { this->block_emptied(empty_block); });
            // The now allocated slot should **always** have a value
            return blocks.back()->allocate(std::forward<Args>(args)...).value();
        }

        /// @function `allocate_array`
        /// @brief Allocates a new array of type `T` with size `length`, where all elements of said array are placed contiguously inside a
        /// single block
        ///
        /// @param `length` The length of the array that will be created
        /// @param `args` The arguments with which every slot in the array will be initialized
        /// @return `Array<T>` The array node which provides a lot of QOL features for handling the array
        template <typename... Args> Array<T> allocate_array(const size_t length, Args &&...args) {
            // Try to allocate in an existing block
            for (size_t i = blocks.size(); i > 0; i--) {
                auto *block_ptr = blocks[i - 1].get();
                if (block_ptr == nullptr) {
                    continue;
                }
                if (block_ptr->get_free_count() >= length) {
                    auto arr = block_ptr->allocate_array(length, std::forward<Args>(args)...);
                    if (arr.has_value()) {
                        return arr.value();
                    }
                }
            }
            // Apply the block mutex, as now definitely a new block will be added one way or the other
            std::lock_guard<std::mutex> lock(blocks_mutex);

            // Try to cerate a block that isnt created yet in the current blocks vector
            for (size_t i = blocks.size(); i > 0; i--) {
                if (blocks[i - 1] != nullptr) {
                    continue;
                }
                const size_t block_capacity = BASE_SIZE << (i - 1);
                if (block_capacity < length) {
                    // Only smaller blocks will follow, so we need a bigger block than this
                    break;
                }
                // Otherwise we can create a new block, that one should fit the array
                blocks[i - 1] = std::make_unique<Block<T>>(i - 1, BASE_SIZE << (i - 1));
                blocks[i - 1]->set_empty_callback([this](Block<T> *empty_block) { this->block_emptied(empty_block); });
                return blocks[i - 1]->allocate_array(length, std::forward<Args>(args)...).value();
            }

            // If all blocks are full, create a new block that has enough space for the array
            size_t block_id = blocks.size();
            while (BASE_SIZE << block_id < length) {
                blocks.push_back(nullptr);
                block_id++;
            }
            const size_t new_size = BASE_SIZE << block_id;
            blocks.emplace_back(std::make_unique<Block<T>>(block_id, new_size));
            blocks.back()->set_empty_callback([this](Block<T> *empty_block) { this->block_emptied(empty_block); });
            // The now allocated array should **always** have a value
            return blocks.back()->allocate_array(length, std::forward<Args>(args)...).value();
        }

        /// @function `reserve`
        /// @brief Reserves enough space in the DIMA tree that at least `n` objects will fit in it. This function only creates the biggest
        /// block in the block list, which holds at least `(n / 2) + BASE_SIZE` elements, as block creation and block filling is done from
        /// the biggest to the smallest blocks. This reduces fragmentation over time and also improves allocation speed, as the biggest
        /// blocks are the most unlikely to be filled up.
        ///
        /// @param `n` The number of objects to reserve
        void reserve(const size_t n) {
            std::lock_guard<std::mutex> lock(blocks_mutex);
            size_t block_index = 0;
            while (BASE_SIZE << block_index < n / 2 + BASE_SIZE) {
                if (blocks.size() == block_index) {
                    // Create empty blocks in the blocks vector which do not point to any real block
                    blocks.push_back(nullptr);
                }
                block_index++;
            }
            blocks.emplace_back(std::make_unique<Block<T>>(block_index, BASE_SIZE << block_index));
            blocks.back()->set_empty_callback([this](Block<T> *empty_block) { this->block_emptied(empty_block); });
        }

      private:
        /// @var `blocks`
        /// @brief A list of all currently active blocks
        std::vector<std::unique_ptr<Block<T>>> blocks;

        /// @var `blocks_mutex`
        /// @brief A mutex to ensure only one thread can modify the blocks at a time
        std::mutex blocks_mutex;

        /// @function `block_emptied`
        /// @brief The callback function which gets executed whenever a block gets emptied
        ///
        /// @param `empty_block` The block which got emptied
        void block_emptied(Block<T> *empty_block) {
            std::lock_guard<std::mutex> lock(blocks_mutex);
            size_t idx = empty_block->get_id();

            // Free the block
            blocks[idx].reset();

            // Remove all empty big blocks bigger than this block from the list
            for (size_t i = blocks.size() - 1; i > idx; i--) {
                if (blocks[i] == nullptr) {
                    blocks.resize(i); // Truncate the vector
                } else {
                    break; // Stop at first non-empty block
                }
            }

            // If only the first block remains and it's empty, clear everything
            if (blocks.size() == 1 && blocks[0] == nullptr) {
                blocks.clear();
            } else {
                // Or, if all blocks are empty, clear everything
                bool every_block_null = true;
                for (const auto &block : blocks) {
                    if (block != nullptr) {
                        every_block_null = false;
                        break;
                    }
                }
                if (every_block_null) {
                    blocks.clear();
                }
            }
        }

        // Here are the non-core public functions. Everything above cannot be removed, these are additional functions publically available
        // to call
      public:
        /// @function `get_allocation_count`
        /// @brief Returns the number of all allocated variables of type `T`
        ///
        /// @return `size_t` The number of all allocated variables
        size_t get_allocation_count() {
            size_t count = 0;
            for (auto &block : blocks) {
                if (block != nullptr) {
                    count += block->get_allocation_count();
                }
            }
            return count;
        }

        /// @function `get_free_count`
        /// @brief Returns the number of all free slots
        ///
        /// @return `size_t` The number of free slots in all blocks
        size_t get_free_count() {
            size_t count = 0;
            for (auto &block : blocks) {
                if (block != nullptr) {
                    count += block->get_free_count();
                }
            }
            return count;
        }

        /// @function `get_capacity`
        /// @brief Returns the total capacity among all DIMA blocks
        ///
        /// @return `size_t` The total capacity among all DIMA blocks
        size_t get_capacity() {
            size_t count = 0;
            for (auto &block : blocks) {
                if (block != nullptr) {
                    count += block->get_capacity();
                }
            }
            return count;
        }

        /// @function `parallel_foreach`
        /// @brief Applies a function to all available slots in parallel
        ///
        /// @param `func` The function to apply
        template <typename Func> void parallel_foreach(Func &&func) {
            // #pragma omp parallel for
            for (size_t i = 0; i < blocks.size(); i++) {
                if (blocks.at(i) != nullptr) {
                    blocks.at(i)->apply_to_all_slots(std::forward<Func>(func));
                }
            }
        }
    };
} // namespace dima
