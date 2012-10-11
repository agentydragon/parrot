#include "tokenizer.h"
#include "common.h"

#include <stdio.h>
#include <ctype.h>

// TODO: english stemming?

static bool is_word_char(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

void for_each_token(char* line, void (*callback)(void* opaque, const char* word, int length), void* opaque) {
#if DEBUG
	printf("tokenizing [%s]\n", line);
	const char* i;
#endif
	char* start = line, *end = line;
	while (*end) {
		*end = tolower(*end);
		if (is_word_char(*end)) {
			end++;
		} else {
			if (start != end) {
#if DEBUG
				printf("[");
				for (i = start; i < end; i++) {
					putchar(*i);
				}
				printf("] ");
#endif

				callback(opaque, start, end - start);
			}
			start = end;
			while (!is_word_char(*start) && *start) {
				start++;
			}
			end = start;
		}
	}

	if (start != end) {
#if DEBUG
		printf("[");
		for (i = start; i < end; i++) {
			putchar(*i);
		}
		printf("] ");
#endif
		callback(opaque, start, end - start);
	}
#if DEBUG
	printf("\n");
#endif

	// TODO: +final token
}
