#ifndef DIMA_H
#define DIMA_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// --- CONFIGURATION ---
#define DIMA_INITIAL_BLOCK_SIZE 16 // Starting block size (2^4)

// --- SLOT STRUCTURE ---
typedef struct {
    void *value;   // Pointer to stored object
    size_t arc;    // Reference counter
    bool occupied; // Is the slot in use?
} DimaSlot;

// --- BLOCK STRUCTURE ---
typedef struct {
    DimaSlot *slots; // List of slots
    size_t size;     // Number of slots
    size_t used;     // Number of occupied slots
} DimaBlock;

// --- HEAD STRUCTURE ---
typedef struct {
    DimaBlock **blocks; // List of block pointers
    size_t block_count; // Number of allocated blocks
} DimaHead;

// --- FUNCTION DECLARATIONS ---
DimaHead *dima_create();
void *dima_allocate(DimaHead *head, size_t obj_size);
void dima_retain(void *ptr);
void dima_release(void *ptr);
void dima_destroy(DimaHead *head);

// --- MACRO SYSTEM FOR TYPE-SAFETY ---
#define DIMA_DECLARE_TYPE(Type)                                                                                                            \
    static inline Type *dima_alloc_##Type(DimaHead *head) {                                                                                \
        return (Type *)dima_allocate(head, sizeof(Type));                                                                                  \
    }

#endif // DIMA_H
