#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define INDEX_FILE "index.dat"

// I expect the count of English words to be in millions. A 32-bit integer should suffice.
typedef uint32_t hash_t;

void error(const char* format, ...);

extern FILE* fortunes;

#define DEBUG false

#endif
