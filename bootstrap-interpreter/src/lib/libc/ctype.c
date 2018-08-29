#include "libc.h"

int islower(int c) {
    return c >= 'a' && c <= 'z';
}

int isupper(int c) {
    return c >= 'A' && c <= 'Z';
}

int isalpha(int c) {
    return islower(c) || isupper(c);
}

int isdigit(int c) {
    return c >= '0' && c <= '9';
}

int isalnum(int c) {
    return isalpha(c) || isdigit(c);
}

int isspace(int c) {
    return c == ' ' || (c >= '\t' && c <= '\r');
}

int iscntrl(int c) {
    return (c >= 0 && c < 32) || c == 127;
}

/*
int isprint(int c) {
    return c >= ' ' && c <= '~';
}

int ispunct(int c) {
    return isprint(c) && !isalnum(c) && c != ' ';
}

int isblank(int c) {
    return c == ' ' || c == '\t';
}
*/
