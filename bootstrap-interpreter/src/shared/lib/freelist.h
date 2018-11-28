void initPool(size_t itemSize, size_t initialCapacity);
void destroyPool(void);
void* allocate(void);
void reclaim(void* element);
size_t getMemoryUsage(void);
