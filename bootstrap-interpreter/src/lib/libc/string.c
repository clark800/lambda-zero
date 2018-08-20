#include "libc.h"

size_t strlen(const char* str) {
    size_t i = 0;
    for (; str[i] != '\0'; i++);
    return i;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    size_t i = 0;
    for (; i < n && s1[i] != '\0'; i++)
        if (s1[i] != s2[i])
            return s1[i] > s2[i] ? 1 : -1;
    return i == n ? 0 : (s2[i] == '\0' ? 0 : -1);
}

const char* strchr(const char* s, int c) {
    for (; s[0] != '\0'; s++)
        if (s[0] == c)
            return s;
    return c == 0 ? s : NULL;
}

/*
char* strcpy(char* dest, const char* src) {
    while ((*dest++ = *src++));
    return dest;
}

char* strcat(char* dest, const char* src) {
    return strcpy(&dest[strlen(dest)], src);
}

size_t strspn(const char* s, const char* accept) {
    for (size_t i = 0; true; i++)
        if (s[i] == '\0' || strchr(accept, s[i]) == NULL)
            return i;
}

size_t strcspn(const char* s, const char* accept) {
    for (size_t i = 0; true; i++)
        if (s[i] == '\0' || strchr(accept, s[i]) != NULL)
            return i;
}

int strcmp(const char* s1, const char* s2) {
    return strncmp(s1, s2, 0);
}

const char* strpbrk(const char* s, const char* accept) {
    for (; s[0] != '\0'; s++)
        if (strchr(accept, s[0]) != NULL)
            return s;
    return NULL;
}
*/
