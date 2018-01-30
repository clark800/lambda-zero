#include <stdlib.h>
#include <assert.h>
#include "array.h"

struct Array {
    size_t capacity;
    size_t length;
    void** elements;
};

Array* newArray(size_t initialCapacity) {
    Array* array = (Array*)malloc(sizeof(Array));
    array->capacity = initialCapacity;
    array->length = 0;
    array->elements = (void**)malloc(initialCapacity * sizeof(void*));
    return array;
}

void deleteArray(Array* array) {
    free(array->elements);
    free(array);
}

void append(Array* array, void* value) {
    if (array->length == array->capacity) {
        array->capacity *= 2;
        size_t newSize = array->capacity * sizeof(void*);
        array->elements = realloc(array->elements, newSize);
    }
    array->elements[array->length++] = value;
}

size_t length(Array* array) {
    return array->length;
}

void* elementAt(Array* array, size_t index) {
    assert(index < array->length);
    return array->elements[index];
}
