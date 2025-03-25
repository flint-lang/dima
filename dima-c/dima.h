#ifndef DIMA_H
#define DIMA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DIMA_BASE_SIZE 16

// === BLOCK MANAGEMENT SYSTEM ===
typedef struct DimaBlock {
    void *slots;
    uint8_t *slot_flags;
    uint32_t *arc_counters;
    size_t slot_size;
    size_t capacity;
    size_t used;
    struct DimaBlock *next;
} DimaBlock;

typedef struct {
    DimaBlock *head;
    size_t type_size;
} DimaHead;

DimaBlock *dima_create_block(size_t slot_size, size_t capacity) {
    DimaBlock *block = (DimaBlock *)malloc(sizeof(DimaBlock));
    block->slots = malloc(slot_size * capacity);
    block->slot_flags = (uint8_t *)calloc(capacity, sizeof(uint8_t)); // Track used slots
    block->arc_counters = (uint32_t *)calloc(capacity, sizeof(uint32_t));
    block->slot_size = slot_size;
    block->capacity = capacity;
    block->used = 0;
    block->next = NULL;
    return block;
}

void dima_free_block(DimaBlock *block) {
    free(block->slots);
    free(block->slot_flags);
    free(block);
}

void dima_init_head(DimaHead *head, size_t type_size) {
    head->type_size = type_size;
    head->head = dima_create_block(type_size, DIMA_BASE_SIZE);
}

// ===  MEMORY MANAGEMENT ===
void *dima_allocate(DimaHead *head) {
    DimaBlock *block = head->head;
    while (block) {
        for (size_t i = 0; i < block->capacity; i++) {
            if (block->slot_flags[i] == 0) { // Found free slot
                block->slot_flags[i] = 1;    // Mark as used
                block->arc_counters[i] = 1;
                block->used++;
                return (char *)block->slots + (i * block->slot_size);
            }
        }
        block = block->next;
    }

    // If no free slot, allocate new block
    DimaBlock *new_block = dima_create_block(head->type_size, head->head->capacity * 2);
    new_block->next = head->head;
    head->head = new_block;
    return dima_allocate(head);
}

void *dima_retain(DimaHead *head, void *ptr) {
    printf("Hello from retain!\n");
    DimaBlock *block = head->head;
    while (block) {
        char *start = (char *)block->slots;
        char *end = start + (block->capacity * block->slot_size);
        if ((char *)ptr >= start && (char *)ptr < end) {
            size_t index = ((char *)ptr - start) / block->slot_size;
            block->arc_counters[index]++;
            break;
        }
        block = block->next;
    }
    return ptr;
}

void dima_release(DimaHead *head, void *ptr) {
    printf("Hello from release!\n");
    DimaBlock *block = head->head;
    while (block) {
        char *start = (char *)block->slots;
        char *end = start + (block->capacity * block->slot_size);
        if ((char *)ptr >= start && (char *)ptr < end) {
            size_t index = ((char *)ptr - start) / block->slot_size;
            block->arc_counters[index]--;
            printf("Arc counter: %d\n", block->arc_counters[index]);
            if (block->arc_counters[index] > 0) {
                return;
            }
            // Fill the slot with zeroes
            memset(ptr, 0, block->slot_size);
            block->slot_flags[index] = 0xFF;
            block->used--;
            if (block->used == 0 && block != head->head) {
                // Remove empty block (except head)
                DimaBlock *prev = head->head;
                while (prev->next != block)
                    prev = prev->next;
                prev->next = block->next;
                dima_free_block(block);
            }
            return;
        }
        block = block->next;
    }
}

bool dima_is_valid(DimaHead *head, void *ptr) {
    DimaBlock *block = head->head;
    while (block) {
        char *start = (char *)block->slots;
        char *end = start + (block->capacity * block->slot_size);
        if ((char *)ptr >= start && (char *)ptr < end) {
            size_t index = ((char *)ptr - start) / block->slot_size;
            return block->slot_flags[index] != 0xFF;
        }
        block = block->next;
    }
    return false;
}

// === AUTO-CLEANUP SYSTEM (RAII-LIKE) ===
#define VAR(T, p) __attribute__((cleanup(dima_cleanup_##T))) T *p
#define REF(T, ptr) dima_retain(&dima_head_##T, ptr)
#define ALLOC(T, name) VAR(T, name) = dima_allocate(&dima_head_##T)
#define VAR_VALID(T, ptr) dima_is_valid(&dima_head_##T, ptr)
#define RELEASE(T, ptr) dima_release(&dima_head_##T, ptr)
#define DEFER_RELEASE(T, ptr) __attribute__((cleanup(dima_defer_cleanup_##T))) T *defer_release_##ptr = ptr

#define DIMA_DEFINE(T)                                                                                                                     \
    static DimaHead dima_head_##T = {0};                                                                                                   \
    void dima_cleanup_##T(T **ptr) {                                                                                                       \
        dima_release(&dima_head_##T, *ptr);                                                                                                \
    }                                                                                                                                      \
    __attribute__((constructor)) static void dima_init_##T() {                                                                             \
        dima_init_head(&dima_head_##T, sizeof(T));                                                                                         \
    }                                                                                                                                      \
    static inline void dima_defer_cleanup_##T(T **ptr) {                                                                                   \
        if (ptr && *ptr) {                                                                                                                 \
            RELEASE(T, *ptr);                                                                                                              \
        }                                                                                                                                  \
    }

#endif // DIMA_H
