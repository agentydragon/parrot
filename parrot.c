#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <unistd.h>
#include <time.h>

#include <stdarg.h>

#include "common.h"
#include "dictionary.h"
#include "fortune-set.h"

void error(const char* format, ...) {
	va_list args;
	printf("Error: ");
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	printf("\n");
}

bool is_word_char(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

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

Dictionary* dictionary;
FILE* fortunes;

void for_each_token(char* line, void* opaque, void (*callback)(void* opaque, const char* word, int length)) {
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
#if DEBUG
	printf("\n");
#endif
}

struct Opaq {
	Dictionary* dictionary;
	uint64_t fortune;
};

void _callback(void* _op, const char* word, int length) {
	(void) word; (void) length;
	struct Opaq* op = _op;
	hash_t hash = get_hash(word, length);
	dictionary_insert_word(op->dictionary, hash, op->fortune);
}

void load_fortune(FILE* file) {
	char buffer[256] = { 0 };

	uint64_t fortune = ftell(file);
	// printf("F #%ld: ", fortune);
	while (!feof(file)) {
		if (!fgets(buffer, sizeof(buffer), file)) {
			if (feof(file)) break;
			error("Error reading!");
			break;
		}
		// Chomp the newline.
		if (buffer[strlen(buffer) - 1] == '\n') {
			buffer[strlen(buffer) - 1] = '\0';
		}
		if (strcmp(buffer, "%") == 0) break;
#if DEBUG
		printf("%s\n", buffer);
#endif
		struct Opaq op = {
			.dictionary = dictionary,
			.fortune = fortune
		};
		void* opaque = &op;
		for_each_token(buffer, opaque, _callback);
	}

	// TODO: +final token
}

struct Opaq2 {
	Dictionary* dictionary;
	FortuneSet* fortune_set;
};

void print_fortune(uint64_t fortune) {
	// printf("Fortune at [%ld]: \n", fortune);
	fseek(fortunes, fortune, SEEK_SET);
	char buffer[256];

	while (!feof(fortunes)) {
		if (!fgets(buffer, sizeof(buffer), fortunes)) {
			if (feof(fortunes)) break;
			error("Error reading!");
			break;
		}
		if (strcmp(buffer, "%\n") == 0) break;
		printf("%s", buffer);
	}
}

float get_word_score(Dictionary* dictionary, hash_t hash) {
	// char buffer[200];
	int word_count;
	// int length;
	uint64_t total;

	dictionary_get_entry(dictionary, hash,/* buffer, &length, sizeof(buffer), */ &word_count);
	total = dictionary_get_words_total(dictionary);

	float l = log((float)word_count / (float)total);
	return l * l;
}

struct Opaq3 {
	FortuneSet* fortune_set;
	Dictionary* dictionary;
	hash_t hash;
};

void _callback3(void* _op, uint64_t fortune) {
	struct Opaq3* op = _op;
#if DEBUG
	printf("%ld ", fortune);
#endif
	fortune_set_add_score(op->fortune_set, fortune, get_word_score(op->dictionary, op->hash));
//	print_fortune(fortune);
}

void _callback2(void* _op, const char* word, int length) {
	struct Opaq2* op = _op;
#if DEBUG
	int i;
#endif
	hash_t hash = get_hash(word, length);

	if (!dictionary_contains_word(op->dictionary, hash)) {
#if DEBUG
		printf("Skip [");
		for (i = 0; i < length; i++) {
			putchar(word[i]);
		}
		printf("].\n");
#endif
		return;
	}
#if DEBUG
	printf("For [");
	for (i = 0; i < length; i++) {
		putchar(word[i]);
	}
	printf("]: \n");
#endif
	struct Opaq3 op3 = {
		.dictionary = op->dictionary,
		.fortune_set = op->fortune_set,
		.hash = hash
	};
	dictionary_for_each_word_fortune(op->dictionary, hash, _callback3, &op3);
#if DEBUG
	printf("\nThat's all.\n");
#endif
}

int build_index() {
	printf("Tokenizing fortunes...\n");
	// TODO: more files...

	while (!feof(fortunes)) {
		load_fortune(fortunes);
	}

	hash_t hashes[100];
	const int count = sizeof(hashes) / sizeof(*hashes);
	int i, word_count;
	printf("Removing top words...\n");
	dictionary_get_top_entries(dictionary, hashes, count);
	for (i = 0; i < count; i++) {
		dictionary_get_entry(dictionary, hashes[i], &word_count);
		dictionary_forget_word(dictionary, hashes[i]);
	}

	printf("Saving index...\n");
	dictionary_save(dictionary, "dictionary.dat");

	printf("Done.\n");

	return 1;
}

// TODO: dilution: score /= delka fortunu v tokenech

void find_similar() {
	dictionary_load(dictionary, "dictionary.dat");

	FortuneSet* fs;
	fortune_set_init(&fs);
	struct Opaq2 op = {
		.dictionary = dictionary,
		.fortune_set = fs
	};

	char find[] = "Linux is better than Windows.";
	for_each_token(find, &op, _callback2);

	print_fortune(fortune_set_pick(fs, 2033149));
	fortune_set_destroy(&fs);
}

int main(int argc, char** argv) {
	(void) argc; (void) argv;
	srand(time(NULL));
	fortunes = fopen("fortunes", "r");
	dictionary_init(&dictionary);

	if (!fortunes) {
		error("Failed to open fortunes file!");
		return 0;
	}

	//build_index();
	find_similar();

	dictionary_destroy(&dictionary); // TODO takeback
	fclose(fortunes);
	return 0;
}
