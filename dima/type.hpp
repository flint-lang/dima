#include "head.hpp"
#include "var.hpp"
#include <utility>

/// @namespace `dima`
/// @brief The `dima` namespace contains all classes used for the DIMA memory management system
namespace dima {

    /// @class `Type`
    /// @brief A base class for all types managed by DIMA
    template <typename T> class Type {
      public:
        /// @function `allocate`
        /// @brief Creates a new variable of type `T` and saves it in one of the blocks
        ///
        /// @param `args` The arguments with which to create the type T slot
        /// @return `Var<T>` A variable node to the allocated object of type `T`
        template <typename... Args> static inline Var<T> allocate(Args &&...args) {
            return head.allocate(std::forward<Args>(args)...);
        }

        /// @function `reserve`
        /// @brief Reserves enough space in the DIMA tree that at least `n` objects will fit in it. This function only creates the biggest
        /// block in the block list, which holds at least `(n / 2) + BASE_SIZE` elements, as block creation and block filling is done from
        /// the biggest to the smallest blocks. This reduces fragmentation over time and also improves allocation speed, as the biggest
        /// blocks are the most unlikely to be filled up.
        ///
        /// @param `n` The number of objects to reserve
        static inline void reserve(const size_t n) {
            head.reserve(n);
        }

        /// @function `get_allocation_count`
        /// @brief Returns the number of all allocated variables of type `T`
        ///
        /// @return `size_t` The number of all allocated variables
        static inline size_t get_allocation_count() {
            return head.get_allocation_count();
        }

        /// @function `get_free_count`
        /// @brief Returns the number of all free slots
        ///
        /// @return `size_t` The number of free slots in all blocks
        static inline size_t get_free_count() {
            return head.get_free_count();
        }

        /// @function `get_capacity`
        /// @brief Get the total slot capacity for this type
        ///
        /// @return `size_t` The slot capacity of this type
        static inline size_t get_capacity() {
            return head.get_capacity();
        }

        /// @function `parallel_foreach`
        /// @brief Applies a function to all available slots in parallel
        ///
        /// @param `func` The function to apply
        template <typename Func> static inline void parallel_foreach(Func &&func) {
            head.parallel_foreach(std::forward<Func>(func));
        }

      private:
        /// @var `head`
        /// @brief The static DIMA head instance for this type
        static inline Head<T> head;
    };
} // namespace dima
