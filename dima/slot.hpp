#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <type_traits>

/// @namespace `dima`
/// @brief The `dima` namespace contains all classes used for the DIMA memory management system
namespace dima {
    static constexpr size_t BASE_SIZE = 16;

    /// @class `Slot`
    /// @brief A slot inside a DIMA block, the slot is the smallest possible value of DIMA, and it only contains a value and the arc counter
    template <typename T, typename = std::enable_if_t<std::is_class_v<T>>> //
    class Slot {
      public:
        Slot() = default;
        Slot(void *owner_ptr) {
            // Only overwride the owner if this slot does not have an owner
            if (this->owner_ptr == nullptr) {
                this->owner_ptr = owner_ptr;
            }
            value_ptr = reinterpret_cast<T *>(&value);
        }

        enum SlotFlags : uint8_t {
            UNUSED = 0, // It's unused when the flags are completely empty
            OCCUPIED = 1,
            OWNED = 2,
            ARRAY_START = 4,
            ARRAY_MEMBER = 8,
            ASYNC = 16,
            OWNED_BY_ENTITY = 32,
        };

        /// @var `flags`
        /// @brief The flags of this slot
        uint8_t flags = UNUSED;

        /// @var `arc`
        /// @brief The reference counter of this slot, to track how many variables are using this slot
        /// @note The ARC is a 24 Bit number in Flint but C++ does not have a 24 bit number natively so we use the next larger number, being
        /// 32 Bit in size
        std::atomic<uint32_t> arc = {0};

        /// @var `owner_ptr`
        /// @brief The pointer to the owner of this slot
        /// @note A slot can be without an owner, this is intentional and important for the defragmentation process
        void *owner_ptr = nullptr;

        /// @var `value`
        /// @brief The value saved on this slot. As long as the reference count is > 0 this will have a value
        typename std::aligned_storage<sizeof(T), alignof(T)>::type value;

        /// @var `value_ptr`
        /// @brief This value pointer is created when the slot is created and it never changes. It only exists to make the `get` function
        /// faster, as there does not need to happen a reinterpret_cast for every single call of `get`
        T *value_ptr;

        /// @var `on_free_callback`
        /// @brief The callback function which gets executed when this slot becomes empty (`arc` becomes 0)
        std::function<void(Slot<T> *)> on_free_callback;

        /// @function `allocate`
        /// @brief Sets the value of this slot to a new value of type `T`, emplaces the created value of type `T` directly in the value
        ///
        /// @param `args` The arguments with which to create the value of type `T`
        template <typename... Args> void allocate(Args &&...args) {
            new (&value) T(std::forward<Args>(args)...);
            flags |= OCCUPIED;
            arc = 1;
        }

        /// @function `retain`
        /// @brief This function is called whenever a new variable gets access to this slot
        void retain() {
            if (is_occupied()) {
                ++arc;
            }
        }

        /// @function `release`
        /// @brief This function is called whenever a variable goes out of scope or is freed in other ways. It reduces the arc and calls the
        /// callback function if this slot becomes empty to let the block this slot is in know that it has been freed
        void release() {
            if (is_occupied() && --arc == 0) {
                get()->~T();
                flags = UNUSED;
                if (on_free_callback) {
                    // Notify that this slot was freed
                    on_free_callback(this);
                }
            }
        }

        /// @function `is_occupied`
        /// @brief Checks whether this slot is occupied with any value
        ///
        /// @return `bool` Whether this slot is occupied with any value
        inline bool is_occupied() const {
            return flags != 0;
        }

        /// @function `is_array_start`
        /// @brief Checks whether this slot is the start of an array
        ///
        /// @return `bool` Whether this slot is the start of an array
        inline bool is_array_start() const {
            return flags & ARRAY_START;
        }

        /// @function `is_array_member`
        /// @brief Checks whether this slot is a member of an array
        ///
        /// @return `bool` Whether this slot is a member of an array
        inline bool is_array_member() const {
            return flags & ARRAY_MEMBER;
        }

        /// @function `get`
        /// @brief Returns the value of this slot. This function should only be called from within the variable node of DIMA, as it doesnt
        /// do any checking of the existence of the value. But it doesnt needs to, because as long as the variable node of DIMA exists, and
        /// it points to this slot, the `arc` of this slot stays > 0 and this means the value stays valid as long as the variable node
        /// exists
        ///
        /// @return `T *` Returns the value saved on this slot direclty
        inline T *get() {
            return value_ptr;
        }
    };
} // namespace dima
