#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>

// I expect the count of English words to be in millions. A 32-bit integer should suffice.
typedef uint32_t hash_t;

void error(const char* format, ...);

#define DEBUG false

#endif
