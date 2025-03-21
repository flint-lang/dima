#pragma once

#include "slot.hpp"

#include <type_traits>

/// @namespace `dima`
/// @brief The `dima` namespace contains all classes used for the DIMA memory management system
namespace dima {

    /// @class `Var`
    /// @brief A variable reference to an element saved within a DimaSlot. When this vaiable access goes out of scope (RAII-based), the ARC
    /// counter of said DIMA slot will be reduced
    template <typename T, typename = std::enable_if_t<std::is_class_v<T>>> //
    class Var {
      public:
        // Destructor
        ~Var() {
            slot->release();
        }
        // Constructor
        explicit Var(Slot<T> *slot) :
            slot(slot) {};

        // Copy constructor
        Var(const Var &other) {
            slot = other.slot;
            slot->retain();
        }
        // Move constructor
        Var(Var &&other) {
            slot = other.slot;
            slot->retain();
        }
        // Copy assignment
        Var &operator=(const Var &other) {
            if (this != &other) {
                Slot<T> *old_slot = slot;
                slot = other.slot;
                slot->retain();
                old_slot->release();
            }
            return *this;
        }
        // Move assignment
        Var &operator=(Var &&other) {
            if (this != &other) {
                Slot<T> *old_slot = slot;
                slot = other.slot;
                slot->retain();
                old_slot->release();
            }
            return *this;
        }

        // This operator makes working with variables a lot easier, as the data saved inside the slot can be forwareded directly
        // These also ensure that the slot itself stays safe and cannot be modified from within this Var class by the user
        T *operator->() {
            return slot->get();
        }
        const T *operator->() const {
            return slot->get();
        }

      private:
        /// @var `slot`
        /// @brief The dima slot this variable refers to
        Slot<T> *slot;

        // Here are the non-core public functions. Everything above cannot be removed, these are additional functions publically available
        // to call. All functions here can be called with the `.` syntax on dima variables directly
      public:
        /// @function `get_arc_count`
        /// @brief Returns the reference count the slot this variable operates on has
        ///
        /// @return `size_t` The reference count of the slot this variable operates on
        size_t get_arc_count() {
            return slot->arc;
        }
    };
} // namespace dima
