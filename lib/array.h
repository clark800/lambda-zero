#ifndef ARRAY_H
#define ARRAY_H

#include <stddef.h>

typedef struct Array Array;
Array* newArray(size_t initialCapacity);
void deleteArray(Array* array);
size_t length(const Array* array);
void append(Array* array, void* value);
void* unappend(Array* array);
void* elementAt(const Array* array, size_t index);

#endif
