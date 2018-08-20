#include <stdlib.h>
#include "util.h"
#include "array.h"
#include "pool.h"

struct Pool {
    size_t itemSize, pageCapacity, pageUsage;
    void* currentPage;
    Array* pages;
};

void appendPage(Pool* pool) {
    pool->currentPage = smalloc(pool->pageCapacity * pool->itemSize);
    append(pool->pages, pool->currentPage);
    pool->pageUsage = 0;
}

Pool* newPool(size_t itemSize, size_t pageCapacity) {
    Pool* pool = (Pool*)smalloc(sizeof(Pool));
    pool->itemSize = itemSize;
    pool->pageCapacity = pageCapacity;
    pool->pages = newArray(32);
    appendPage(pool);
    return pool;
}

void deletePool(Pool* pool) {
    for (size_t i = 0; i < length(pool->pages); i++)
        free(elementAt(pool->pages, i));
    deleteArray(pool->pages);
    free(pool);
}

void* acquire(Pool* pool) {
    if (pool->pageUsage == pool->pageCapacity)
       appendPage(pool);
    return (void*)(&((char*)(pool->currentPage))
        [pool->itemSize * pool->pageUsage++]);
}
