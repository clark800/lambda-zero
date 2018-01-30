#include <stdio.h>
#include <stdlib.h>
#include "readfile.h"

char* readfile(FILE* stream) {
    size_t length = 0;
    size_t bufferLength = READ_CHUNK_SIZE;
    size_t bytesRead = 0;
    char* buffer = (char*)malloc(bufferLength * sizeof(char));

    do {
        length += (size_t)bytesRead * sizeof(char);
        if (length == bufferLength) {
            bufferLength += READ_CHUNK_SIZE;
            buffer = realloc(buffer, bufferLength * sizeof(char));
        }
        bytesRead = fread(buffer + length, sizeof(char),
            (bufferLength - length), stream);
    } while (bytesRead > 0);

    if (length == bufferLength)
        buffer = realloc(buffer, (length + 1) * sizeof(char));
    buffer[length] = '\0';
    return buffer;
}
