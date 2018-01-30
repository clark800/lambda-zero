#ifndef READFILE_H
#define READFILE_H

#include <stdio.h>

#define READ_CHUNK_SIZE 16384

char* readfile(FILE* stream);

#endif
