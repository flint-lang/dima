#ifndef DIMA_H
#define DIMA_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum : uint8_t {
    DIMA_UNUSED = 0,
    DIMA_OCCUPIED = 1,
    DIMA_ARRAY_START = 2,
    DIMA_ARRAY_MEMBER = 4,
} dima_slot_flags_t;

typedef struct dima_slot_t {
    dima_slot_flags_t flags;
    uint32_t arc;
    char value[];
} dima_slot_t;

typedef struct dima_block_t {
    size_t type_size;
    size_t capacity;
    size_t used;
    size_t pinned_count;
    size_t first_free_slot_id;
    dima_slot_t slots[];
} dima_block_t;

typedef struct dima_head_t {
    size_t type_size;
    const void *default_value;
    size_t block_count;
    dima_block_t *blocks[];
} dima_head_t;

#define DIMA_BASE_CAPACITY 16
#define DIMA_GROWTH_FACTOR 11

#define container_of(ptr, type, member) (type *)((char *)ptr - offsetof(type, member))

#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

// === AUTO-CLEANUP SYSTEM (RAII-LIKE) ===
#define VAR(T, p) __attribute__((cleanup(dima_cleanup_##T))) T *p
#define REF(T, ptr) (T *)dima_retain(ptr)
#define ALLOC(T, name) VAR(T, name) = (T *)dima_allocate(&dima_head_##T)
#define GET_CAPACITY(T) dima_get_active_capacity(dima_head_##T)
#define GET_USED_COUNT(T) dima_get_used_count(dima_head_##T)
#define RESERVE(T, N) dima_reserve(&dima_head_##T, N)
#define VAR_VALID(ptr) dima_is_valid(ptr)
#define RELEASE(T, ptr) dima_release(&dima_head_##T, ptr)
#define DEFER_RELEASE(T, ptr) __attribute__((cleanup(dima_defer_cleanup_##T))) T *defer_release_##ptr = ptr

#define DIMA_DEFINE(T, ...)                                                                                                                \
    static dima_head_t *dima_head_##T = NULL;                                                                                              \
    static T dima_default_value_##T = {__VA_ARGS__};                                                                                       \
    void dima_cleanup_##T(T **ptr) {                                                                                                       \
        dima_release(&dima_head_##T, *ptr);                                                                                                \
    }                                                                                                                                      \
    __attribute__((constructor)) static void dima_init_##T() {                                                                             \
        dima_head_##T = dima_init_head(&dima_default_value_##T, sizeof(T));                                                                \
    }                                                                                                                                      \
    static inline void dima_defer_cleanup_##T(T **ptr) {                                                                                   \
        if (ptr && *ptr) {                                                                                                                 \
            RELEASE(T, *ptr);                                                                                                              \
        }                                                                                                                                  \
    }

/// @function `dima_get_block_capacity`
/// @brief Calculates the block capacity of the given index of the block
///
/// @param `index` The index of the block to get the capacity from
/// @return `size_t` The capacity of the block at the given index
size_t dima_get_block_capacity(const size_t index);

/// @function `dima_init_head`
/// @brief Initializes the head with the given value size
///
/// @param `default_value` The default value to initialize the head with
/// @param `type_size` The size of the type stored in this dima tree
/// @return `dima_head_t *` The initialized head
dima_head_t *dima_init_head(const void *default_value, const size_t type_size);

/// @function `dima_create_block`
/// @brief Creates a new block from the given slot size and capacity
///
/// @param `type_size` The size of the type stored in this dima tree
/// @param `capacity` The capacity the newly created block needs to hold
/// @return `dima_block_t *` The created block
dima_block_t *dima_create_block(const size_t type_size, const size_t capacity);

/// @function `dima_allocate_in_block`
/// @brief Checks for a new value in the given block and returns the pointer to the slot if a value was found
///
/// @param `block` The block in which to "allocate" the new value
/// @return `dima_slot_t *` A pointer to the "allocated" slot
dima_slot_t *dima_allocate_in_block(dima_block_t *block);

/// @function `dima_allocate`
/// @brief Tries to "allocate" a new value in the head. If no block has space for a new value then a new block will be created
///
/// @param `head_ref` The pointer to the variable holding the pointer to the head, to allow relocations of the head to take effect in the
///                   variable too.
/// @return `void *` A pointer to the allocated value contained inside the slot
void *dima_allocate(dima_head_t **head_ref);

/// @function `dima_reserve`
/// @brief Reserves enough space in the head tree that it has a capacity to hold n values in the least possible amount of blocks
///
/// @param `head_ref` The pointer to the variable pointing to the head of this dima tree
/// @param `n` The capacity to reserve the tree for
void dima_reserve(dima_head_t **head_ref, const size_t n);

/// @function `dima_get_active_capacity`
/// @brief Returns the overall allocated capacity of the given dima tree head
///
/// @param `head` The head of the dima tree to get the capacity from
/// @return `size_t` The capacity of the dima tree
size_t dima_get_active_capacity(const dima_head_t *head);

/// @function `dima_get_used_count`
/// @brief Returns how many slots are in active use within the given dima tree
///
/// @param `head` The head of the dima tree to get the used count from
/// @return `size_t` How many slots are in active use within this dima tree
size_t dima_get_used_count(const dima_head_t *head);

/// @function `dima_retain`
/// @brief Used to retain a reference to the passed-in pointer's slot
///
/// @param `value` The pointer to the value stored inside the slot
/// @return `void *` The passed in pointer returned for chainability, but the arc of the slot has been increased
void *dima_retain(void *value);

/// @function `dima_release`
/// @brief Used to release the given value from the slot it is contained in and decreases the ARC of the slot. If it was the last slot in
/// the block and the used count of the block falls to 0 then the block will be freed to, potentially reallocating the head of the dima
/// tree.
///
/// @param `head_ref` Pointer to the variable pointing to the head of the dima tree
/// @param `value` The pointer to the value allocated in one of the blocks
void dima_release(dima_head_t **head_ref, void *value);

/// @function `dima_is_valid`
/// @brief Checks whether the given pointer to the allocated value is valid. It is valid if the slot it contains has the `DIMA_OCCUPIED`
/// flag set to 1
///
/// @param `value` The slot value to check
/// @return `bool` Whether the given slot value is valid
bool dima_is_valid(const void *value);

#ifdef DIMA_IMPLEMENTATION

size_t dima_get_block_capacity(size_t index) {
    size_t cap = DIMA_BASE_CAPACITY;
    for (size_t j = 0; j < index; j++) {
        // Integer mul/div with ceil rounding to approximate float growth
        cap = (cap * DIMA_GROWTH_FACTOR + 9) / 10;
    }
    return cap;
}

dima_head_t *dima_init_head(const void *default_value, const size_t type_size) {
    dima_head_t *new_head = (dima_head_t *)malloc(sizeof(dima_head_t));
    *new_head = (dima_head_t){
        .type_size = type_size,
        .default_value = default_value,
        .block_count = 0,
    };
    return new_head;
}

dima_block_t *dima_create_block(const size_t type_size, const size_t capacity) {
    const size_t slot_size = sizeof(dima_slot_t) + type_size;
    dima_block_t *block = (dima_block_t *)malloc(sizeof(dima_block_t) + slot_size * capacity);
    memset(block->slots, 0, capacity * slot_size);
    block->type_size = type_size;
    block->capacity = capacity;
    block->used = 0;
    block->pinned_count = 0;
    block->first_free_slot_id = 0;
    return block;
}

dima_slot_t *dima_allocate_in_block(dima_block_t *block) {
    const size_t slot_size = sizeof(dima_slot_t) + block->type_size;
    for (size_t i = block->first_free_slot_id; i < block->capacity; i++) {
        dima_slot_t *slot = (dima_slot_t *)((char *)block->slots + slot_size * i);
        if (slot->flags == DIMA_UNUSED) {
            slot->flags = DIMA_OCCUPIED;
            slot->arc = 1;
            block->used++;
            block->first_free_slot_id = i + 1;
            return slot;
        }
    }
    return NULL;
}

void *dima_allocate(dima_head_t **head_ref) {
    dima_head_t *head = *head_ref;
    dima_slot_t *slot_ptr = NULL;
    if (UNLIKELY(head->blocks == NULL)) {
        // Create the first block
        *head_ref = (dima_head_t *)realloc(head, sizeof(dima_head_t) + sizeof(dima_block_t *));
        head = *head_ref;
        head->block_count = 1;
        head->blocks[0] = dima_create_block(head->type_size, DIMA_BASE_CAPACITY);
        slot_ptr = dima_allocate_in_block(head->blocks[0]);
    } else {
        // Try to find free slot
        for (size_t i = head->block_count; i > 0; i--) {
            dima_block_t *block = head->blocks[i - 1];
            if (UNLIKELY(block == NULL)) {
                continue;
            }
            if (LIKELY(block->used == block->capacity)) {
                continue;
            }
            // If came here, there definitely is a free slot, so allocation wont fail
            slot_ptr = dima_allocate_in_block(block);
            break;
        }
        if (UNLIKELY(slot_ptr == NULL)) {
            // Check if an a block can be created within the blocks array
            for (size_t i = head->block_count; i > 0; i--) {
                if (UNLIKELY(head->blocks[i - 1] == NULL)) {
                    head->blocks[i - 1] = dima_create_block(head->type_size, dima_get_block_capacity(i - 1));
                    slot_ptr = dima_allocate_in_block(head->blocks[i - 1]);
                    break;
                }
            }
        }
        if (UNLIKELY(slot_ptr == NULL)) {
            // No free slot, allocate new block by reallocating the head
            *head_ref = (dima_head_t *)realloc(head, sizeof(dima_head_t) + sizeof(dima_block_t *) * (head->block_count + 1));
            head = *head_ref;
            head->blocks[head->block_count] = dima_create_block(head->type_size, dima_get_block_capacity(head->block_count));
            head->block_count++;
            // There definitely will be a free slot now
            slot_ptr = dima_allocate_in_block(head->blocks[head->block_count - 1]);
        }
    }
    // Copy the default value into the slot
    memcpy(slot_ptr->value, head->default_value, head->type_size);
    return slot_ptr->value;
}

void dima_reserve(dima_head_t **head_ref, const size_t n) {
    dima_head_t *head = *head_ref;
    if (UNLIKELY(n <= DIMA_BASE_CAPACITY)) {
        return;
    }
    // We need at least one block, so lets create it
    if (UNLIKELY(head->block_count == 0)) {
        *head_ref = (dima_head_t *)realloc(head, sizeof(dima_head_t) + sizeof(dima_block_t *));
        head = *head_ref;
        head->blocks[0] = NULL;
        head->block_count = 1;
    }
    // Start at block index 1
    size_t block_index = 1;
    while (DIMA_BASE_CAPACITY << block_index < (n * 10) / DIMA_GROWTH_FACTOR + DIMA_BASE_CAPACITY) {
        // Resize the blocks array if bigger blocks are needed
        if (head->block_count == block_index) {
            *head_ref = (dima_head_t *)realloc(head, sizeof(dima_head_t) + sizeof(dima_block_t *) * (block_index + 1));
            head = *head_ref;
            head->blocks[block_index] = NULL;
            head->block_count++;
        }
        block_index++;
    }
    // TODO: Potentially dead code, needs testing
    if (head->block_count >= block_index) {
        // Dont do anything, as the head already has enough space for what we need
        return;
    }
    *head_ref = (dima_head_t *)realloc(head, sizeof(dima_head_t) + sizeof(dima_block_t *) * (block_index + 1));
    head = *head_ref;
    head->blocks[head->block_count] = dima_create_block(head->type_size, dima_get_block_capacity(block_index));
    head->block_count++;
}

size_t dima_get_active_capacity(const dima_head_t *head) {
    size_t capacity = 0;
    for (size_t i = 0; i < head->block_count; i++) {
        if (head->blocks[i] != NULL) {
            capacity += dima_get_block_capacity(i);
        }
    }
    return capacity;
}

size_t dima_get_used_count(const dima_head_t *head) {
    size_t used_count = 0;
    for (size_t i = 0; i < head->block_count; i++) {
        dima_block_t *block = head->blocks[i];
        if (block != NULL) {
            used_count += block->used;
        }
    }
    return used_count;
}

void *dima_retain(void *value) {
    dima_slot_t *slot = container_of(value, dima_slot_t, value);
    slot->arc++;
    return value;
}

void dima_release(dima_head_t **head_ref, void *value) {
    dima_slot_t *slot = container_of(value, dima_slot_t, value);
    assert(slot->arc > 0);
    slot->arc--;
    if (LIKELY(slot->arc > 0)) {
        // Do not apply all the below checks since no block is potentially freed
        return;
    }
    // Start at the biggest block because it has the most slots, so it is the most likely to contain the slot
    dima_head_t *head = *head_ref;
    const size_t slot_size = sizeof(dima_slot_t) + head->type_size;
    for (size_t i = head->block_count; i > 0; i--) {
        if (UNLIKELY(head->blocks[i - 1] == NULL)) {
            continue;
        }
        dima_block_t *block = head->blocks[i - 1];
        char *start = (char *)block->slots;
        char *end = start + (block->capacity * slot_size);
        if (UNLIKELY((char *)value >= start && (char *)value < end)) {
            // This is the block containing the freed slot
            assert(block->used > 0);
            block->used--;
            // Fill the slot with zeroes
            memset(slot, 0, slot_size);
            size_t index = ((char *)slot - start) / slot_size;
            if (block->first_free_slot_id > index) {
                block->first_free_slot_id = index;
            }
            if (UNLIKELY(block->used == 0)) {
                // Remove empty block
                free(block);
                head->blocks[i - 1] = NULL;
                // Shrink the blocks array if the last block was freed up to the first block thats not null
                if (UNLIKELY(i == head->block_count)) {
                    // Check how many empty blocks there are to calculate the new size
                    size_t new_size = head->block_count - 1;
                    for (; new_size > 0; new_size--) {
                        if (head->blocks[new_size - 1] != NULL) {
                            break;
                        }
                    }
                    // Realloc the head to the new size
                    *head_ref = (dima_head_t *)realloc(head, sizeof(dima_head_t) + sizeof(dima_block_t *) * (new_size));
                    head = *head_ref;
                    head->block_count = new_size;
                }
            }
            return;
        }
    }
}

bool dima_is_valid(const void *value) {
    if (value == NULL) {
        return false;
    }
    dima_slot_t *slot = container_of(value, dima_slot_t, value);
    return slot->flags & DIMA_OCCUPIED;
}

#endif // DIMA_IMPLEMENTATION

#endif // DIMA_H
