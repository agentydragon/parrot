#include "lcs.h"

#include <stdlib.h>

// Returns the length of the longest common substring of A and B.
// Runs in O(a_len * b_len), eats O(a_len * b_len) memory.
// TODO: reimplement using suffix trees
int longest_common_substring(const hash_t* a, uint64_t a_len, const hash_t* b, uint64_t b_len) {
	uint64_t i, j;

#if DEBUG
	printf("longest_common_substring\n");
	printf("A = [ ");
	for (i = 0; i < a_len; i++) {
		printf("%X ", a[i]);
	}
	printf("];\nB = [ ");
	for (i = 0; i < b_len; i++) {
		printf("%X ", b[i]);
	}
	printf("];\n");
#endif

	uint64_t *L;
	L = malloc(sizeof(*L) * a_len * b_len);
	uint64_t z = 0;

	for (i = 0; i < a_len; i++) {
		for (j = 0; j < b_len; j++) {
			if (a[i] == b[j]) {
				if (i == 0 || j == 0) {
					L[i*b_len + j] = 1;
				} else {
					L[i*b_len + j] = L[(i-1)*b_len + j-1] + 1;
				}

				if (L[i*b_len + j] > z) {
					z = L[i*b_len + j];
				}
			} else {
				L[i*b_len + j] = 0;
			}
		}
	}

	free(L);

#if DEBUG
	printf("result: %ld\n", z);
#endif

	return z;
}
