#pragma once

#include "slot.hpp"
#include "var.hpp"

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
        Block(const size_t n) :
            slots(n) {
            for (auto &slot : slots) {
                slot.on_free_callback = [this](Slot<T> *freed_slot) { this->slot_freed(freed_slot); };
            }
        }

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
        /// @return `std::optional<size_t>` The index of the next empy slot within this block, nullopt if this block is full
        std::optional<size_t> find_empty_slot() const {
            if (occupied_slots == slots.size()) {
                return std::nullopt;
            }
            for (size_t i = 0; i < slots.size(); ++i) {
                if (!slots.at(i).value.has_value()) {
                    return i;
                }
            }
            return std::nullopt;
        }

        /// @function `allocate`
        /// @brief Creates a new variable of type `T` and saves it in this block. If this block is full, it returns nullopt without saving
        /// it here
        ///
        /// @param `args` The arguments with which to create the type T slot
        /// @return `std::optional<Var<T>>` A variable node to the allocated object of type `T`, nullopt if this block is full
        template <typename... Args> std::optional<Var<T>> allocate(Args &&...args) {
            std::optional<size_t> slotIdx = find_empty_slot();
            if (!slotIdx.has_value()) {
                return std::nullopt;
            }
            slots.at(slotIdx.value()).allocate(std::forward<Args>(args)...);
            occupied_slots++;
            return Var<T>(&slots.at(slotIdx.value()));
        }

      private:
        /// @var `slots`
        /// @brief A list of all slots this block contains
        std::vector<Slot<T>> slots;

        /// @var `on_empty_callback`
        /// @brief The callback that gets executed when this block becomes empty
        std::function<void(Block<T> *)> on_empty_callback;

        /// @var `occupied_slots`
        /// @brief The number of occupied slots within this block
        size_t occupied_slots = 0;

        /// @function `slot_freed`
        /// @brief This function gets called from a slot that has been freed
        ///
        /// @param `freed_slot` The slot which has been freed;
        void slot_freed(Slot<T> *freed_slot) {
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
            return slots.size() - occupied_slots;
        }

        /// @function `get_capacity`
        /// @brief Returns the total capacity of this block
        ///
        /// @return `size_t` The total capacity of this block
        size_t get_capacity() {
            return slots.size();
        }

        /// @function `apply_to_all_slots`
        /// @brief Applies a function to all slots, if the slots have a value
        ///
        /// @param `func` The function to apply
        template <typename Func> void apply_to_all_slots(Func &&func) {
            for (auto &slot : slots) {
                if (slot.value.has_value()) {
                    std::forward<Func>(func)(slot.value.value());
                }
            }
        }
    };
} // namespace dima
