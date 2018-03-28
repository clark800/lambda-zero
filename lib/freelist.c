#include <assert.h>
#include <stddef.h>
#include "pool.h"
#include "freelist.h"

// NEXT is a pointer to a linked list of unallocated slots, where the elements
// of the linked list are stored in the slots themselves and contain no data
static void* NEXT = NULL;

// we keep a marker in every allocated slot before the stored data so that
// we can detect if a slot is double-reclaimed since this situation can lead
// to very confusing behavior
void* MARKER = (void*)(0xDEFACED);

static Pool* POOL = NULL;
static size_t SIZE = 0;
static size_t COUNT = 0;

void initPool(size_t itemSize, size_t initialCapacity) {
    assert(POOL == NULL);
    SIZE = sizeof(void*) + itemSize;
    POOL = newPool(SIZE, initialCapacity);
    NEXT = POOL;
}

void destroyPool() {
    deletePool(POOL);
}

size_t getMemoryUsage() {
    return COUNT * SIZE;
}

void* mark(void* slot) {
    *(void**)slot = MARKER;
    return &(((void**)slot)[1]);
}

void* allocate() {
    COUNT += 1;
    if (NEXT == POOL)
        return mark(acquire(POOL));
    void* head = NEXT;
    NEXT = *(void**)NEXT;
    return mark(head);
}

void* unmark(void* allocated) {
    assert(((void**)allocated)[-1] == MARKER);  // detect double reclaim
    return &(((void**)allocated)[-1]);
}

void reclaim(void* allocated) {
    COUNT -= 1;
    void* tail = NEXT;
    NEXT = unmark(allocated);
    *(void**)NEXT = tail;
}
