#pragma once

#include <functional>
#include <optional>
#include <type_traits>

/// @namespace `dima`
/// @brief The `dima` namespace contains all classes used for the DIMA memory management system
namespace dima {

    /// @class `Slot`
    /// @brief A slot inside a DIMA block, the slot is the smallest possible value of DIMA, and it only contains a value and the arc counter
    template <typename T, typename = std::enable_if_t<std::is_class_v<T>>> //
    class Slot {
      public:
        /// @var `arc`
        /// @brief The reference counter of this slot, to track how many variables are using this slot
        size_t arc = 0;

        /// @var `value`
        /// @brief The value saved on this slot. As long as the reference count is > 0 this will have a value
        std::optional<T> value;

        /// @var `on_free_callback`
        /// @brief The callback function which gets executed when this slot becomes empty (`arc` becomes 0)
        std::function<void(Slot<T> *)> on_free_callback;

        /// @function `allocate`
        /// @brief Sets the value of this slot to a new value of type `T`, emplaces the created value of type `T` directly in the value
        ///
        /// @param `args` The arguments with which to create the value of type `T`
        template <typename... Args> void allocate(Args &&...args) {
            value.emplace(std::forward<Args>(args)...);
            arc = 1;
        }

        /// @function `retain`
        /// @brief This function is called whenever a new variable gets access to this slot
        void retain() {
            if (value.has_value()) {
                ++arc;
            }
        }

        /// @function `release`
        /// @brief This function is called whenever a variable goes out of scope or is freed in other ways. It reduces the arc and calls the
        /// callback function if this slot becomes empty to let the block this slot is in know that it has been freed
        void release() {
            if (value.has_value() && --arc == 0) {
                value = std::nullopt;
                if (on_free_callback) {
                    // Notify that this slot was freed
                    on_free_callback(this);
                }
            }
        }

        /// @function `get`
        /// @brief Returns the value of this slot. This function should only be called from within the variable node of DIMA, as it doesnt
        /// do any checking of the existence of the value. But it doesnt needs to, because as long as the variable node of DIMA exists, and
        /// it points to this slot, the `arc` of this slot stays > 0 and this means the value stays valid as long as the variable node
        /// exists
        ///
        /// @return `T *` Returns the value saved on this slot direclty
        T *get() {
            return &value.value();
        }
    };
} // namespace dima
