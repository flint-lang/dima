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
            for (auto block_it = blocks.begin(); block_it != blocks.end(); ++block_it) {
                if (*block_it != nullptr && block_it->get()->get_free_count() > 0) {
                    auto var = block_it->get()->allocate(std::forward<Args>(args)...);
                    if (var.has_value()) {
                        return var.value();
                    }
                }
            }

            // If full, create a new block with 2x size of the last one, a new block definitely has space for a new variable
            std::lock_guard<std::mutex> lock(blocks_mutex);
            const size_t block_id = blocks.size();
            const size_t new_size = blocks.empty() ? BASE_SIZE : (BASE_SIZE << block_id);
            blocks.emplace_back(std::make_unique<Block<T>>(block_id, new_size));
            blocks.back()->set_empty_callback([this](Block<T> *empty_block) { this->block_emptied(empty_block); });
            // The now allocated slot should **always** have a value
            return blocks.back()->allocate(std::forward<Args>(args)...).value();
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
