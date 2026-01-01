#pragma once

#include "array.hpp"
#include "slot.hpp"
#include "var.hpp"

#include <algorithm>
#include <bitset>
#include <functional>
#include <optional>
#include <type_traits>
#include <vector>

/// @namespace `dima`
/// @brief The `dima` namespace contains all classes used for the DIMA memory management system
namespace dima {

    /// @class `Block`
    /// @brief A memory block containing multiple DIMA slots
    template <typename T, typename = std::enable_if_t<std::is_class_v<T>>> //
    class Block {
      public:
        Block(const uint32_t block_id, const size_t n) :
            block_id(block_id),
            capacity(n),
            slots(n) {
            free_slots.resize(n / BASE_SIZE);
            for (auto &slot : slots) {
                slot.on_free_callback = [this](Slot<T> *freed_slot) { this->slot_freed(freed_slot); };
            }
        }

      private:
        /// @var `block_id`
        /// @brief The id of this block. Is also equal to the index of this block in the blocks vector
        uint32_t block_id;

        /// @var `capacity`
        /// @brief The capacity of this block
        uint32_t capacity = 0;

        /// @var `occupied_slots`
        /// @brief The number of occupied slots within this block
        uint32_t occupied_slots = 0;

        //// @var `pinned_count`
        /// @brief The number of pinned slots within this block
        uint32_t pinned_count = 0;

        /// @var `last_non_full_set`
        /// @brief A simple number to cache what the last non-full slot is, all slots to the left of this index are considered full. This
        /// variable always "points" to the first non-full bitset
        uint32_t last_non_full_set = 0;

        /// @var `slots`
        /// @brief A list of all slots this block contains
        std::vector<Slot<T>> slots;

        /// @var `free_slots`
        /// @brief A vector to track the free slots to drastrically reduce the number of times `find_empty_slot` needs to be
        /// called
        std::vector<std::bitset<BASE_SIZE>> free_slots;

        /// @var `on_empty_callback`
        /// @brief The callback that gets executed when this block becomes empty
        std::function<void(Block<T> *)> on_empty_callback;

      public:
        /// @function `set_empty_callback`
        /// @brief Sets the callback function of this block to execute when this block becommes empty
        ///
        /// @param `callback` The function to execute when this block becomes empty
        void set_empty_callback(std::function<void(Block<T> *)> callback) {
            on_empty_callback = std::move(callback);
        }

        /// @function `find_empty_slot`
        /// @brief Finds the index of the next empty slot within this block, or nullopt if this block is full
        ///
        /// @return `int` The index of the next empy slot within this block, -1 if this block is full
        int find_empty_slot() {
            if (occupied_slots == capacity) {
                return -1;
            }
            const size_t free_slots_size = free_slots.size();
            for (size_t i = last_non_full_set; i < free_slots_size; i++) {
                auto &set = free_slots[i];
                // Skip if all bits are 1 (all occupied)
                if (set.all()) {
                    last_non_full_set = i;
                    continue;
                }

                // Find the first 0 bit
                const uint64_t data = set.to_ullong();
                uint64_t inverted = ~data;
                // Mask off any bits beyond BASE_SIZE
                inverted &= (1ULL << BASE_SIZE) - 1;
                const unsigned long bit_idx = __builtin_ctzl(inverted);
                return i * BASE_SIZE + bit_idx;
            }
            return -1;
        }

        /// @function `allocate`
        /// @brief Creates a new variable of type `T` and saves it in this block. If this block is full, it returns nullopt without saving
        /// it here
        ///
        /// @param `args` The arguments with which to create the type T slot
        /// @return `std::optional<Var<T>>` A variable node to the allocated object of type `T`, nullopt if this block is full
        template <typename... Args> std::optional<Var<T>> allocate(Args &&...args) {
            int idx = find_empty_slot();
            if (idx < 0) {
                return std::nullopt;
            }
            slots[idx].allocate(std::forward<Args>(args)...);
            free_slots[idx / BASE_SIZE][idx % BASE_SIZE] = true;
            occupied_slots++;
            return Var<T>(&slots[idx]);
        }

        /// @function `get_id`
        /// @brief Returns the id of this block
        ///
        /// @return `size_t` The id of this block
        size_t get_id() {
            return block_id;
        }

        /// @function `allocate_array`
        /// @brief Allocates an array of slots within this block, the array must be contiguous and surrounded by two free slots
        ///
        /// @param `length` The size of the array to create within this block
        /// @param `args` The arguments with which to construct the slots
        /// @return `std::optional<Array<T>>` The variable pointing to the start of the array, nullopt if the array does not fit into this
        /// block
        template <typename... Args> std::optional<Array<T>> allocate_array(const uint32_t length, Args &&...args) {
            // Need length+2 contiguous slots (array + padding on both ends)
            const uint32_t required = length + 2;

            if (required > capacity || occupied_slots + required > capacity) {
                return std::nullopt; // Not enough space in the block
            }

            // Find a contiguous space large enough
            uint32_t contiguous_count = 0;
            uint32_t start_position = 0;

            // Scan through bitsets to find a contiguous region
            for (uint32_t i = 0; i < free_slots.size(); i++) {
                const std::bitset<BASE_SIZE> &set = free_slots[i];

                // Process each bit in the current bitset
                for (uint32_t j = 0; j < BASE_SIZE; j++) {
                    uint32_t actual_idx = i * BASE_SIZE + j;
                    if (actual_idx >= capacity)
                        break; // Don't go beyond capacity

                    if (!set[j]) { // If slot is free
                        if (contiguous_count == 0) {
                            start_position = actual_idx;
                        }
                        contiguous_count++;

                        if (contiguous_count == required) {
                            // Found enough contiguous space

                            // Allocate the slots (skip the first padding slot)
                            for (uint32_t k = 1; k <= length; k++) {
                                const uint32_t idx = start_position + k;
                                free_slots[idx / BASE_SIZE][idx % BASE_SIZE] = true;
                                slots[idx].allocate(std::forward<Args>(args)...);
                            }

                            // Update occupied slots count
                            occupied_slots += length;

                            // Create and return the Array
                            return Array<T>(slots.begin() + start_position + 1, length);
                        }
                    } else {
                        // Reset counter when we encounter an occupied slot
                        contiguous_count = 0;
                    }
                }
            }

            // No suitable contiguous space found
            return std::nullopt;
        }

      private:
        /// @function `slot_freed`
        /// @brief This function gets called from a slot that has been freed
        ///
        /// @param `freed_slot` The slot which has been freed;
        void slot_freed(Slot<T> *freed_slot) {
            // Calculate the index by finding the offset from the start of the slots vector
            uint32_t idx = freed_slot - &slots[0];

            // Mark the slot as free
            const uint32_t free_set_idx = idx / BASE_SIZE;
            free_slots[free_set_idx][idx & BASE_SIZE] = false;

            // Update the index tracking variable for cache optimization
            if (free_set_idx < last_non_full_set) {
                last_non_full_set = free_set_idx;
            }
            occupied_slots--;
            if (occupied_slots == 0 && on_empty_callback) {
                // Notif that this block is now empty
                on_empty_callback(this);
            }
        }

        // Here are the non-core public functions. Everything above cannot be removed, these are additional functions publically available
        // to call
      public:
        /// @function `get_allocation_count`
        /// @brief Returns the number of occupied slots
        ///
        /// @return `size_t` The number of occupied slots in this block
        size_t get_allocation_count() {
            return occupied_slots;
        }

        /// @function `get_free_count`
        /// @brief Returns the number of free slots
        ///
        /// @return `size_t` The number of free slots in this block
        size_t get_free_count() {
            return capacity - occupied_slots;
        }

        /// @function `get_capacity`
        /// @brief Returns the total capacity of this block
        ///
        /// @return `size_t` The total capacity of this block
        size_t get_capacity() {
            return capacity;
        }

        /// @function `apply_to_all_slots`
        /// @brief Applies a function to all slots, if the slots have a value
        ///
        /// @param `func` The function to apply
        template <typename Func> void apply_to_all_slots(Func &&func) {
            for (auto &slot : slots) {
                if (slot.is_occupied()) {
                    std::forward<Func>(func)(reinterpret_cast<T &>(slot.value));
                }
            }
        }
    };
} // namespace dima
