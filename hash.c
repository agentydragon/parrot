#include "hash.h"

// A FNV hash over a token
hash_t get_hash(const char* word, int length) {
	int i;
	hash_t j = 2166136261;
	for (i = 0; i < length; i++) {
		j ^= word[i];
		j *= 16777619;
	}
	return j;
}
