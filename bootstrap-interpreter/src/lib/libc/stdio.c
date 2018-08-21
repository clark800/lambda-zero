#include "libc.h"

static size_t toBytes(ssize_t result) {
    return result >= 0 ? (size_t)result : 0;
}

static int fileno(FILE* stream) {
    return (int)(intptr_t)stream;
}

FILE* fopen(const char* filename, const char* mode) {
    assert(mode[0] == 'r' && mode[1] == '\0');
    return (FILE*)(intptr_t)open(filename, O_RDONLY);
}

int fclose(FILE *stream) {
    return close(fileno(stream));
}

size_t fread(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    return toBytes(read(fileno(stream), ptr, nmemb * size));
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
    return toBytes(write(fileno(stream), ptr, nmemb * size));
}

int fputs(const char* str, FILE* stream) {
    return (int)fwrite(str, sizeof(char), strlen(str), stream);
}

int fputc(int c, FILE* stream) {
    unsigned char ch = (unsigned char)c;
    return fwrite(&ch, 1, 1, stream) == 1 ? c : EOF;
}

int fgetc(FILE* stream) {
    char buffer[2];
    ssize_t bytes = read(fileno(stream), buffer, 1);
    return bytes == 1 ? buffer[0] : EOF;
}

void setbuf(FILE* stream, char* buffer) {
    assert(stream != (FILE*)(-1) && buffer == NULL);
}

/*
int fflush(FILE* stream) {
    return fsync(fileno(stream));
}
*/
