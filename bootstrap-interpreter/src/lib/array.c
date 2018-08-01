#include <assert.h>
#include <stdlib.h>
#include "util.h"
#include "array.h"

struct Array {
    size_t capacity;
    size_t length;
    void** elements;
};

Array* newArray(size_t initialCapacity) {
    Array* array = (Array*)smalloc(sizeof(Array));
    array->capacity = initialCapacity;
    array->length = 0;
    array->elements = (void**)smalloc(initialCapacity * sizeof(void*));
    return array;
}

void deleteArray(Array* array) {
    if (array->elements != NULL)
        free(array->elements);      // may be NULL if capacity is zero
    free(array);
}

void append(Array* array, void* value) {
    if (array->length == array->capacity) {
        array->capacity = array->capacity == 0 ? 1 : 2 * array->capacity;
        size_t newSize = array->capacity * sizeof(void*);
        array->elements = realloc(array->elements, newSize);
        if (array->elements == NULL)
            error("\nError: out of memory\n");
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
