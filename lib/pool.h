#ifndef POOL_H
#define POOL_H

#include <stddef.h>

typedef struct Pool Pool;
Pool* newPool(size_t itemSize, size_t pageCapacity);
void deletePool(Pool* pool);
void* acquire(Pool* pool);

#endif
