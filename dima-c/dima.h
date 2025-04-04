#ifndef DIMA_H
#define DIMA_H

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DIMA_BASE_SIZE 16

typedef enum : uint8_t {
    UNUSED = 0,
    OCCUPIED = 1,
    ARRAY_START = 2,
    ARRAY_MEMBER = 4,
} SlotFlags;

typedef struct {
    size_t slot_size;
    size_t capacity;
    size_t used;
    size_t first_free_slot_id;
    uint8_t *slot_flags;
    uint32_t *arc_counters;
    void *slots;
} DimaBlock;

typedef struct {
    DimaBlock **blocks;
    size_t block_count;
    size_t slot_size;
    void *default_value;
} DimaHead;

#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

// === AUTO-CLEANUP SYSTEM (RAII-LIKE) ===
#define VAR(T, p) __attribute__((cleanup(dima_cleanup_##T))) T *p
#define REF(T, ptr) dima_retain(&dima_head_##T, ptr)
#define ALLOC(T, name) VAR(T, name) = (T *)dima_allocate(&dima_head_##T)
#define GET_CAPACITY(T) dima_get_active_capacity(&dima_head_##T)
#define GET_USED_COUNT(T) dima_get_used_count(&dima_head_##T)
#define RESERVE(T, N) dima_reserve(&dima_head_##T, N)
#define VAR_VALID(T, ptr) dima_is_valid(&dima_head_##T, ptr)
#define RELEASE(T, ptr) dima_release(&dima_head_##T, ptr)
#define DEFER_RELEASE(T, ptr) __attribute__((cleanup(dima_defer_cleanup_##T))) T *defer_release_##ptr = ptr

#define DIMA_DEFINE(T, ...)                                                                                                                \
    static DimaHead dima_head_##T = {0};                                                                                                   \
    static T dima_default_value_##T = {__VA_ARGS__};                                                                                       \
    void dima_cleanup_##T(T **ptr) {                                                                                                       \
        dima_release(&dima_head_##T, *ptr);                                                                                                \
    }                                                                                                                                      \
    __attribute__((constructor)) static void dima_init_##T() {                                                                             \
        dima_init_head(&dima_head_##T, sizeof(T));                                                                                         \
        dima_head_##T.default_value = &dima_default_value_##T;                                                                             \
    }                                                                                                                                      \
    static inline void dima_defer_cleanup_##T(T **ptr) {                                                                                   \
        if (ptr && *ptr) {                                                                                                                 \
            RELEASE(T, *ptr);                                                                                                              \
        }                                                                                                                                  \
    }

void dima_init_head(DimaHead *head, size_t slot_size);

DimaBlock *dima_create_block(size_t slot_size, size_t capacity);

void dima_free_block(DimaBlock *block);

void *dima_allocate_in_block(DimaBlock *block);

void *dima_allocate(DimaHead *head);

void dima_reserve(DimaHead *head, size_t n);

size_t dima_get_active_capacity(DimaHead *head);

size_t dima_get_used_count(DimaHead *head);

void *dima_retain(DimaHead *head, void *ptr);

void dima_release(DimaHead *head, void *ptr);

bool dima_is_valid(DimaHead *head, void *ptr);

#if defined(DIMA_IMPLEMENTATION)

#ifdef LIKELYHOOD_TESTING
#define LC(N)                                                                                                                              \
    static long likelyhood_counter_##N = 0;                                                                                                \
    static long likelyhood_sum_##N = 0;
#define LC_LIKELY(N)                                                                                                                       \
    likelyhood_counter_##N++;                                                                                                              \
    likelyhood_sum_##N++;
#define LC_UNLIKELY(N)                                                                                                                     \
    likelyhood_counter_##N--;                                                                                                              \
    likelyhood_sum_##N++;
#define LC_PRINT(N) printf("Likelyhood %d: %ld / %ld\n", N, likelyhood_counter_##N, likelyhood_sum_##N);
#else
#define LC(N)
#define LC_LIKELY(N)
#define LC_UNLIKELY(N)
#define LC_PRINT(N)
#endif

void dima_init_head(DimaHead *head, size_t slot_size) {
    head->slot_size = slot_size;
    head->blocks = NULL;
    head->block_count = 0;
    head->default_value = NULL;
}

DimaBlock *dima_create_block(size_t slot_size, size_t capacity) {
    // When creating the block, all the slots should be contained *in* the block
    DimaBlock *block = (DimaBlock *)malloc(sizeof(DimaBlock) + slot_size * capacity);
    // The slots array starts right after the actual block struct in memory
    block->slots = block + 1;
    // Set all slot contents to zeroes
    memset(block->slots, 0, capacity * slot_size);
    block->slot_flags = (uint8_t *)calloc(capacity, sizeof(uint8_t)); // Track used slots
    block->arc_counters = (uint32_t *)calloc(capacity, sizeof(uint32_t));
    block->slot_size = slot_size;
    block->capacity = capacity;
    block->used = 0;
    block->first_free_slot_id = 0;
    return block;
}

void dima_free_block(DimaBlock *block) {
    free(block->slot_flags);
    free(block->arc_counters);
    free(block);
}

void *dima_allocate_in_block(DimaBlock *block) {
    for (size_t j = block->first_free_slot_id; j < block->capacity; j++) {
        if (block->slot_flags[j] == UNUSED) { // Found free slot
            block->slot_flags[j] = OCCUPIED;  // Mark as used
            block->arc_counters[j] = 1;
            block->used++;
            block->first_free_slot_id = j + 1;
            return (char *)block->slots + (j * block->slot_size);
        }
    }
    return NULL;
}

LC(0)
LC(1)
LC(2)

void *dima_allocate(DimaHead *head) {
    void *slot_ptr = NULL;
    if (UNLIKELY(head->blocks == NULL)) {
        LC_UNLIKELY(0)
        // Create the first block
        head->blocks = (DimaBlock **)calloc(1, sizeof(DimaBlock *));
        head->blocks[0] = dima_create_block(head->slot_size, DIMA_BASE_SIZE);
        head->block_count = 1;
        slot_ptr = dima_allocate_in_block(head->blocks[0]);
    } else {
        LC_LIKELY(0)
        // Try to find free slot
        for (size_t i = head->block_count; i > 0; i--) {
            DimaBlock *block = head->blocks[i - 1];
            if (UNLIKELY(block == NULL)) {
                continue;
            }
            if (LIKELY(block->used == block->capacity)) {
                LC_LIKELY(1)
                continue;
            }
            LC_UNLIKELY(1);
            // If came here, there definitely is a free slot, so allocation wont fail
            slot_ptr = dima_allocate_in_block(block);
            break;
        }
        if (UNLIKELY(slot_ptr == NULL)) {
            // Check if an a block can be created within the blocks array
            for (size_t i = head->block_count; i > 0; i--) {
                if (UNLIKELY(head->blocks[i - 1] == NULL)) {
                    head->blocks[i - 1] = dima_create_block(head->slot_size, (size_t)(DIMA_BASE_SIZE << (i - 1)));
                    slot_ptr = dima_allocate_in_block(head->blocks[i - 1]);
                    break;
                }
            }
        }
        if (UNLIKELY(slot_ptr == NULL)) {
            LC_UNLIKELY(2)
            // No free slot, allocate new block
            // First, change the blocks array and resize it
            head->blocks = (DimaBlock **)realloc(head->blocks, (head->block_count + 1) * sizeof(DimaBlock *));
            head->blocks[head->block_count] = dima_create_block(head->slot_size, (size_t)(DIMA_BASE_SIZE << head->block_count));
            head->block_count++;
            // There definitely will be a free slot now
            slot_ptr = dima_allocate_in_block(head->blocks[head->block_count - 1]);
        } else {
            LC_LIKELY(2)
        }
    }
    // Copy the default value into the slot
    memcpy(slot_ptr, head->default_value, head->slot_size);
    return slot_ptr;
}

void dima_reserve(DimaHead *head, size_t n) {
    if (UNLIKELY(n <= DIMA_BASE_SIZE)) {
        return;
    }
    // We need at least one block, so lets create it
    if (UNLIKELY(head->block_count == 0)) {
        head->blocks = (DimaBlock **)malloc(sizeof(DimaBlock *));
        head->blocks[0] = (DimaBlock *)NULL;
        head->block_count = 1;
    }
    // Start at block index 1
    size_t block_index = 1;
    while (DIMA_BASE_SIZE << block_index < n / 2 + DIMA_BASE_SIZE) {
        // Resize the blocks array if bigger blocks are needed
        if (head->block_count == block_index) {
            head->blocks = (DimaBlock **)realloc(head->blocks, sizeof(DimaBlock *) * (block_index + 1));
            head->blocks[block_index] = (DimaBlock *)NULL;
            head->block_count++;
        }
        block_index++;
    }
    if (head->block_count > block_index) {
        // Dont do anything, as the head already has enough space for what we need
        return;
    }
    head->blocks = (DimaBlock **)realloc(head->blocks, sizeof(DimaBlock *) * (block_index + 1));
    head->block_count++;
    head->blocks[block_index] = dima_create_block(head->slot_size, DIMA_BASE_SIZE << block_index);
}

size_t dima_get_active_capacity(DimaHead *head) {
    size_t capacity = 0;
    for (size_t i = 0; i < head->block_count; i++) {
        if (head->blocks[i] != (DimaBlock *)NULL) {
            capacity += DIMA_BASE_SIZE << i;
        }
    }
    return capacity;
}

size_t dima_get_used_count(DimaHead *head) {
    size_t used_count = 0;
    for (size_t i = 0; i < head->block_count; i++) {
        DimaBlock *block = head->blocks[i];
        if (block != (DimaBlock *)NULL) {
            used_count += block->used;
        }
    }
    return used_count;
}

LC(3)
LC(4)

void *dima_retain(DimaHead *head, void *ptr) {
    // Start at the biggest block because it has the most slots, so it is the most likely to contain the slot
    for (size_t i = head->block_count; i > 0; i--) {
        DimaBlock *block = head->blocks[i - 1];
        if (UNLIKELY(block == NULL)) {
            LC_UNLIKELY(3)
            continue;
        }
        LC_LIKELY(3)
        char *start = (char *)block->slots;
        char *end = start + (block->capacity * block->slot_size);
        if (LIKELY((char *)ptr >= start && (char *)ptr < end)) {
            LC_LIKELY(4)
            size_t index = ((char *)ptr - start) / block->slot_size;
            block->arc_counters[index]++;
            return ptr;
        }
        LC_UNLIKELY(4)
    }
    // It should *never* get here
    printf("dima_retain failed\n");
    return NULL;
}

LC(5)
LC(6)
LC(7)
LC(8)

void dima_release(DimaHead *head, void *ptr) {
    // Start at the biggest block because it has the most slots, so it is the most likely to contain the slot
    for (size_t i = head->block_count; i > 0; i--) {
        if (UNLIKELY(head->blocks[i - 1] == (DimaBlock *)NULL)) {
            LC_UNLIKELY(5)
            continue;
        }
        DimaBlock *block = head->blocks[i - 1];
        LC_LIKELY(5)
        char *start = (char *)block->slots;
        char *end = start + (block->capacity * block->slot_size);
        if (UNLIKELY((char *)ptr >= start && (char *)ptr < end)) {
            LC_UNLIKELY(6)
            size_t index = ((char *)ptr - start) / block->slot_size;
            // printf("release: %ld : %d\n", index, block->arc_counters[index]);
            block->arc_counters[index]--;
            if (LIKELY(block->arc_counters[index] > 0)) {
                LC_LIKELY(7)
                return;
            }
            LC_UNLIKELY(7)
            // Fill the slot with zeroes
            memset(ptr, 0, block->slot_size);
            block->slot_flags[index] = UNUSED;
            block->used--;
            if (block->first_free_slot_id > index) {
                block->first_free_slot_id = index;
            }
            if (UNLIKELY(block->used == 0)) {
                LC_UNLIKELY(8)
                // Remove empty block
                dima_free_block(block);
                head->blocks[i - 1] = (DimaBlock *)NULL;
                // Shrink the blocks array if the last block was freed up to the first block thats not null
                if (UNLIKELY(i == head->block_count)) {
                    size_t new_size = head->block_count - 1;
                    for (; new_size > 0; new_size--) {
                        if (head->blocks[new_size - 1] != NULL) {
                            break;
                        }
                    }
                    if (UNLIKELY(new_size == 0)) {
                        // Completely free the blocks array if all blocks were freed
                        head->block_count = 0;
                        free(head->blocks);
                        head->blocks = NULL;
                    } else {
                        // Otherwise, shrink the blocks array
                        head->block_count = new_size;
                        head->blocks = (DimaBlock **)realloc(head->blocks, new_size * sizeof(DimaBlock *));
                    }
                }
            } else {
                LC_LIKELY(8)
            }
            return;
        }
        LC_LIKELY(6)
    }
}

LC(9)
LC(10)
LC(11)

bool dima_is_valid(DimaHead *head, void *ptr) {
    // Start at the biggest block because it has the most slots, so it is the most likely to contain the slot
    if (UNLIKELY(head->blocks == NULL)) {
        LC_UNLIKELY(9)
        return false;
    }
    LC_LIKELY(9)
    for (size_t i = head->block_count; i > 0; i--) {
        DimaBlock *block = head->blocks[i - 1];
        if (UNLIKELY(block == NULL)) {
            LC_UNLIKELY(10)
            continue;
        }
        LC_LIKELY(10)
        char *start = (char *)block->slots;
        char *end = start + (block->capacity * block->slot_size);
        if (LIKELY((char *)ptr >= start && (char *)ptr < end)) {
            LC_LIKELY(11)
            size_t index = ((char *)ptr - start) / block->slot_size;
            return block->slot_flags[index] == OCCUPIED;
        }
        LC_UNLIKELY(11)
    }
    return false;
}

void print_likelyhoods() {
#ifndef LIKELYHOOD_TESTING
    return;
}
#else
    printf("\n");

    // Print likelyhood diagnostics
    LC_PRINT(0)
    LC_PRINT(1)
    LC_PRINT(2)
    LC_PRINT(3)
    LC_PRINT(4)
    LC_PRINT(5)
    LC_PRINT(6)
    LC_PRINT(7)
    LC_PRINT(8)
    LC_PRINT(9)
    LC_PRINT(10)
    LC_PRINT(11)
}
#endif

#endif // DIMA_IMPLEMENTATION

#endif // DIMA_H
