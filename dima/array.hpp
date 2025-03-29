#include "slot.hpp"
#include "var.hpp"

#include <cassert>
#include <cstddef>
#include <vector>

namespace dima {
    template <typename T> class Array {
      private:
        using slot_iterator = typename std::vector<Slot<T>>::iterator;
        using const_slot_iterator = typename std::vector<Slot<T>>::const_iterator;
        slot_iterator first_slot;
        size_t length;

        void release_all() {
            const slot_iterator end = first_slot + length;
            for (auto it = first_slot; it != end; ++it) {
                (*it).release();
            }
        }

        void retain_all() {
            const slot_iterator end = first_slot + length;
            for (auto it = first_slot; it != end; ++it) {
                (*it).retain();
            }
        }

      public:
        // Custom iterator class that wraps the slot iterator
        class iterator {
          private:
            slot_iterator it;

          public:
            // Standard iterator type definitions
            using difference_type = std::ptrdiff_t;
            using value_type = T;
            using pointer = T *;
            using reference = T &;
            using iterator_category = std::random_access_iterator_tag;

            iterator(slot_iterator it) :
                it(it) {}

            // Dereference returns Var<T>
            Var<T> operator*() {
                (*it).retain();
                return Var<T>(&(*it));
            }

            // For -> operator, we need a proxy since Var<T> is returned by value
            struct arrow_proxy {
                T *ptr;
                arrow_proxy(T *p) :
                    ptr(p) {}
                T *operator->() {
                    return ptr;
                }
            };

            arrow_proxy operator->() const {
                return arrow_proxy((*it).get());
            }

            // Pre-increment
            iterator &operator++() {
                ++it;
                return *this;
            }

            // Post-increment
            iterator operator++(int) {
                iterator tmp = *this;
                ++it;
                return tmp;
            }

            // Equality operators
            bool operator==(const iterator &other) const {
                return it == other.it;
            }
            bool operator!=(const iterator &other) const {
                return it != other.it;
            }

            // Addition and subtraction
            iterator &operator+=(difference_type n) {
                it += n;
                return *this;
            }
            iterator &operator-=(difference_type n) {
                it -= n;
                return *this;
            }
            iterator operator+(difference_type n) const {
                return iterator(it + n);
            }
            iterator operator-(difference_type n) const {
                return iterator(it - n);
            }
            difference_type operator-(const iterator &other) const {
                return it - other.it;
            }

            // Indexing
            Var<T> operator[](difference_type n) const {
                return Var<T>(&(*(it + n)));
            }
        };

        // Custom const_iterator class
        class const_iterator {
          private:
            const_slot_iterator it;

          public:
            using difference_type = std::ptrdiff_t;
            using value_type = const Var<T>;
            using pointer = const Var<T> *;
            using reference = const Var<T>;
            using iterator_category = std::forward_iterator_tag;

            const_iterator(const_slot_iterator it) :
                it(it) {}

            // Const dereference
            Var<T> operator*() const {
                return Var<T>(const_cast<Slot<T> *>(&(*it)));
            }

            // Const -> operator
            struct arrow_proxy {
                const T *ptr;
                arrow_proxy(const T *p) :
                    ptr(p) {}
                const T *operator->() {
                    return ptr;
                }
            };

            arrow_proxy operator->() const {
                return arrow_proxy((*it).get());
            }

            // Pre-increment
            const_iterator &operator++() {
                ++it;
                return *this;
            }

            // Post-increment
            const_iterator operator++(int) {
                const_iterator tmp = *this;
                ++it;
                return tmp;
            }

            // Equality operators
            bool operator==(const const_iterator &other) const {
                return it == other.it;
            }
            bool operator!=(const const_iterator &other) const {
                return it != other.it;
            }

            // Addition and subtraction
            const_iterator &operator+=(difference_type n) {
                it += n;
                return *this;
            }
            const_iterator &operator-=(difference_type n) {
                it -= n;
                return *this;
            }
            const_iterator operator+(difference_type n) const {
                return const_iterator(it + n);
            }
            const_iterator operator-(difference_type n) const {
                return const_iterator(it - n);
            }
            difference_type operator-(const const_iterator &other) const {
                return it - other.it;
            }

            // Indexing
            Var<T> operator[](difference_type n) const {
                return Var<T>(const_cast<Slot<T> *>(&(*(it + n))));
            }
        };

        ~Array() {
            release_all();
        }

        // Constructor with explicit length
        Array(slot_iterator first, size_t len) :
            first_slot(first),
            length(len) {}

        // Copy constructor
        Array(const Array &other) :
            first_slot(other.first_slot),
            length(other.length) {
            retain_all();
        }

        // Move constructor
        Array(Array &&other) noexcept :
            first_slot(other.first_slot),
            length(other.length) {
            other.first_slot = slot_iterator(); // Invalidate the source
            other.length = 0;
        }

        // Copy assignment
        Array &operator=(const Array &other) {
            if (this != &other) {
                release_all();
                first_slot = other.first_slot;
                length = other.length;
            }
            retain_all();
            return *this;
        }

        // Move assignment
        Array &operator=(Array &&other) noexcept {
            if (this != &other) {
                release_all();
                first_slot = other.first_slot;
                length = other.length;
                other.first_slot = slot_iterator();
                other.length = 0;
            }
            return *this;
        }
        // Element access
        Var<T> operator[](size_t index) {
            assert(index < length);
            slot_iterator it = first_slot + index;
            (*it).retain();
            return Var<T>(&(*it));
        }

        const Var<T> operator[](size_t index) const {
            assert(index < length);
            slot_iterator it = first_slot + index;
            (*it).retain();
            return Var<T>(&(*it));
        }

        size_t size() const {
            return length;
        }

        // Iterators
        iterator begin() {
            return iterator(first_slot);
        }
        iterator end() {
            return iterator(first_slot + length);
        }
        const_iterator begin() const {
            return const_iterator(first_slot);
        }
        const_iterator end() const {
            return const_iterator(first_slot + length);
        }
        const_iterator cbegin() const {
            return const_iterator(first_slot);
        }
        const_iterator cend() const {
            return const_iterator(first_slot + length);
        }
    };
} // namespace dima
