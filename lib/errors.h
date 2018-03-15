#ifndef ERRORS_H
#define ERRORS_H

extern int VERBOSITY;
typedef const char* strings[];
void errorArray(int count, const char* strs[]);
void error(const char* type, const char* message);

#endif
