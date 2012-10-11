#include "common.h"
#include "dictionary.h"
#include "fortune-set.h"
#include "tokenizer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <time.h>

#include <stdarg.h>

void error(const char* format, ...) {
	va_list args;
	printf("Error: ");
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	printf("\n");
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

typedef struct {
	Dictionary* dictionary;
	uint64_t fortune;
	int length;
} fortune_token_callback_data;

static void _fortune_token_callback(void* _opaque, const char* word, int length) {
	(void) word; (void) length;
	fortune_token_callback_data* data = _opaque;
	hash_t hash = get_hash(word, length);
	dictionary_insert_word(data->dictionary, hash, data->fortune);
	data->length++;
}

static void load_fortune(FILE* file) {
	char buffer[256] = { 0 };

	uint64_t fortune = ftell(file);
	fortune_token_callback_data callback_data = {
		.dictionary = dictionary,
		.fortune = fortune,
		.length = 0
	};
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
		for_each_token(buffer, &callback_data, _fortune_token_callback);
	}

	dictionary_insert_fortune(dictionary, fortune, callback_data.length);
}

void print_fortune(uint64_t fortune) {
#if DEBUG
	printf("Fortune at [%ld]: \n", fortune);
#endif
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

	dictionary_get_entry(dictionary, hash, &word_count);
	total = dictionary_get_words_total(dictionary);

	float l = log((float)word_count / (float)total);
	return l * l;
}

typedef struct {
	FortuneSet* fortune_set;
	Dictionary* dictionary;
	hash_t hash;
} matching_fortune_callback_data;

static void _matching_fortune_callback(void* _opaque, uint64_t fortune) {
	matching_fortune_callback_data* data = _opaque;
#if DEBUG
	printf("%ld ", fortune);
#endif
	float score = get_word_score(data->dictionary, data->hash);
	fortune_set_add_score(data->fortune_set, fortune, score);
//	print_fortune(fortune);
}

typedef struct {
	Dictionary* dictionary;
	FortuneSet* fortune_set;
} input_token_callback_data;

static void _input_token_callback(void* _opaque, const char* word, int length) {
	input_token_callback_data* data = _opaque;
#if DEBUG
	int i;
#endif
	hash_t hash = get_hash(word, length);

	if (!dictionary_contains_word(data->dictionary, hash)) {
#if DEBUG
		printf("not in dictionary: [");
		for (i = 0; i < length; i++) {
			putchar(word[i]);
		}
		printf("].\n");
#endif
		return;
	}
#if DEBUG
	printf("[");
	for (i = 0; i < length; i++) {
		putchar(word[i]);
	}
	printf("]: \n");
#endif
	dictionary_for_each_word_fortune(data->dictionary, hash, _matching_fortune_callback, & (matching_fortune_callback_data) {
		.dictionary = data->dictionary,
		.fortune_set = data->fortune_set,
		.hash = hash
	});
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
	dictionary_save(dictionary, "index.dat");

	printf("Done.\n");

	return 1;
}

// TODO: dilution: score /= delka fortunu v tokenech

uint64_t pick_for_input(char* input, uint64_t avoid) {
	uint64_t fortune;
	FortuneSet* fs;
	fortune_set_init(&fs);

	for_each_token(input, & (input_token_callback_data) {
		.dictionary = dictionary,
		.fortune_set = fs
	}, _input_token_callback);

	if (fortune_set_is_empty(fs)) {
		error("TODO: pick a random fortune");
		exit(1);
	} else {
		fortune = fortune_set_pick(fs, avoid);
	}

	fortune_set_destroy(&fs);
	return fortune;
}

void read_fortune(uint64_t fortune, char* buffer) {
	strcpy(buffer, "");
	fseek(fortunes, fortune, SEEK_SET);
	char line[256];

	while (!feof(fortunes)) {
		if (!fgets(line, sizeof(line), fortunes)) {
			if (feof(fortunes)) break;
			error("Error reading!");
			break;
		}
		if (strcmp(line, "%\n") == 0) break;
		strcpy(buffer + strlen(buffer), line);
	}
}

void find_similar(bool do_continuous, int continuous_interval) {
	dictionary_load(dictionary, "index.dat");

	char input[1024]; // TODO: tohle je zle maximum.

	while (!feof(stdin)) {
		if (!fgets(input + strlen(input), sizeof(input) - strlen(input), stdin)) {
			if (feof(stdin)) break;
			error("Failed to read from stdin!");
			break;
		}
	}

	if (do_continuous) {
		uint64_t fortune = pick_for_input(input, -1);
		do {
			print_fortune(fortune);
			read_fortune(fortune, input);
			fortune = pick_for_input(input, fortune);
			sleep(continuous_interval);
		} while (true);
	} else {
		print_fortune(pick_for_input(input, -1));
	}
}

// TODO: specifikovat soubor?
// TODO: continuous?
void show_usage(const char* program) {
	printf("Usage: %s [--rebuild-index | --continuous]\n", program);
	printf("\t--rebuild-index: rebuild the fortune index\n");
	printf("\t--continuous: \"stream-of-consciousness mode\"\n");
}

int main(int argc, char** argv) {
	int f = 0;
	bool do_build_index = false, do_continuous = false;

	(void) argc; (void) argv;

	srand(time(NULL));
	fortunes = fopen("fortunes", "r");

	if (!fortunes) {
		error("Failed to open fortunes file!");
		return 3;
	}

	dictionary_init(&dictionary);

	if (argc > 1) {
		if (argc == 2 && strcmp(argv[1], "--rebuild-index") == 0) {
			do_build_index = true;
		} else if (argc == 2 && strcmp(argv[1], "--continuous") == 0) {
			do_continuous = true;
		} else {
			show_usage(argv[0]);
			f = 1;
			goto exit;
		}
	}

	if (do_build_index) {
		if (!build_index()) {
			error("Failed to build index.");
			f = 2;
			goto exit;
		}
		goto exit;
	}

	find_similar(do_continuous, 10);

exit:
	dictionary_destroy(&dictionary); // TODO takeback
	fclose(fortunes);
	return f;
}
