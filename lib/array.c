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
    if (array->elements != NULL)
        free(array->elements);      // may be NULL if capacity is zero
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

void* unappend(Array* array) {
    assert(array->length > 0);
    return array->elements[array->length--];
}

size_t length(const Array* array) {
    return array->length;
}

void* elementAt(const Array* array, size_t index) {
    assert(index < array->length);
    return array->elements[index];
}
