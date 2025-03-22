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
            std::lock_guard<std::mutex> lock(blocks_mutex);
            // Try to allocate in an existing block
            for (auto block_it = blocks.begin(); block_it != blocks.end(); ++block_it) {
                if (*block_it != nullptr) {
                    auto var = block_it->get()->allocate(std::forward<Args>(args)...);
                    if (var.has_value()) {
                        return var.value();
                    }
                } else {
                    size_t new_size = (BaseSize << std::distance(blocks.begin(), block_it));
                    *block_it = std::make_unique<Block<T>>(new_size);
                    block_it->get()->set_empty_callback([this](Block<T> *empty_block) { this->block_emptied(empty_block); });
                    return block_it->get()->allocate(std::forward<Args>(args)...).value();
                }
            }

            // If full, create a new block with 2x size of the last one, a new block definitely has space for a new variable
            size_t new_size = blocks.empty() ? BASE_SIZE : (BASE_SIZE << blocks.size());
            blocks.emplace_back(std::make_unique<Block<T>>(new_size));
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
            for (size_t i = 0; i < blocks.size(); i++) {
                if (blocks.at(i).get() == empty_block) {
                    blocks.at(i).reset();
                    break;
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
            std::lock_guard<std::mutex> lock(blocks_mutex);
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
            std::lock_guard<std::mutex> lock(blocks_mutex);
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
            std::lock_guard<std::mutex> lock(blocks_mutex);
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
#pragma omp parallel for
            for (size_t i = 0; i < blocks.size(); i++) {
                if (blocks.at(i) != nullptr) {
                    blocks.at(i)->apply_to_all_slots(std::forward<Func>(func));
                }
            }
        }
    };
} // namespace dima
