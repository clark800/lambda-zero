#include <assert.h>
#include <stddef.h>
#include "pool.h"
#include "freelist.h"

static Pool* POOL = NULL;
static void* NEXT = NULL;
static size_t SIZE = 0;
static size_t COUNT = 0;

void initPool(size_t itemSize, size_t initialCapacity) {
    assert(POOL == NULL);
    POOL = newPool(itemSize, initialCapacity);
    NEXT = NULL;
    SIZE = itemSize;
}

void destroyPool() {
    deletePool(POOL);
}

size_t getMemoryUsage() {
    return COUNT * SIZE;
}

void* allocate() {
    COUNT += 1;
    if (NEXT == NULL)
        return acquire(POOL);
    void* head = NEXT;
    NEXT = *(void**)NEXT;
    return head;
}

void reclaim(void* element) {
    COUNT -= 1;
    void* tail = NEXT;
    NEXT = element;
    *(void**)NEXT = tail;
}
