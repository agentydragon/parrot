#include "common.h"
#include "index.h"
#include "fortune-set.h"
#include "tokenizer.h"
#include "hash.h"
#include "build-index.h"
#include "lcs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>

static void read_fortune(uint64_t fortune, char** _buffer, uint64_t *buffer_cap);

#include <stdarg.h>

void error(const char* format, ...) {
	va_list args;
	printf("Error: ");
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	printf("\n");
}

typedef struct {
	hash_t* array;
	uint64_t length;
	uint64_t capacity;
} tokenize_into_array_data;

static void _tokenize_into_array_callback(void* _opaque, const char* word, int length) {
	tokenize_into_array_data* data = _opaque;
	if (data->length == data->capacity) {
		data->capacity *= 2;
		data->array = realloc(data->array, sizeof(hash_t) * data->capacity);
	}
	data->array[data->length++] = get_hash(word, length);
}

static void tokenize_into_array(char* input, hash_t** _array, uint64_t* _length, uint64_t* _capacity) {
	tokenize_into_array_data data = {
		.length = 0,
		.capacity = 8
	};
	data.array = malloc(sizeof(hash_t) * data.capacity);
	for_each_token(input, _tokenize_into_array_callback, &data);	
	*_array = data.array;
	*_length = data.length;
	*_capacity = data.capacity;
}

FILE* fortunes;

static void print_fortune(uint64_t fortune) {
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

static float get_word_score(Index* index, hash_t hash) {
	int word_count;
	uint64_t total;

	index_get_entry(index, hash, &word_count);
	total = index_get_words_total(index);

	float l = log((float)word_count / (float)total);
	return l * l;
}

typedef struct {
	FortuneSet* fortune_set;
	Index* index;
	hash_t hash;
} matching_fortune_callback_data;

static void _matching_fortune_callback(void* _opaque, uint64_t fortune) {
	matching_fortune_callback_data* data = _opaque;
#if DEBUG
	printf("%ld ", fortune);
#endif
	float score = get_word_score(data->index, data->hash);
	fortune_set_add_score(data->fortune_set, fortune, score);
}

static void tokenize_fortune_into_array(uint64_t fortune, hash_t** _tokens, uint64_t* _size, uint64_t* _capacity) {
	char buffer[256] = { 0 };

	fseek(fortunes, fortune, SEEK_SET);
	tokenize_into_array_data data = {
		.length = 0,
		.capacity = 8
	};
	data.array = malloc(sizeof(hash_t) * data.capacity);
	while (!feof(fortunes)) {
		if (!fgets(buffer, sizeof(buffer), fortunes)) {
			if (feof(fortunes)) break;
			error("Error reading!");
			break;
		}
		// Chomp the newline.
		if (buffer[strlen(buffer) - 1] == '\n') {
			buffer[strlen(buffer) - 1] = '\0';
		}
		if (strcmp(buffer, "%") == 0) break;
		for_each_token(buffer, _tokenize_into_array_callback, &data);	
	}
	*_tokens = data.array;
	*_size = data.length;
	*_capacity = data.capacity;
}

typedef struct {
	Index* index;
	const hash_t* tokens;
	uint64_t tokens_size;
} adjust_score_data;

static float _adjust_score(void* _opaque, uint64_t fortune, float former_score) {
	float score = former_score;
	adjust_score_data* data = _opaque;

	hash_t* tokens;
	uint64_t tokens_size, tokens_cap;

	tokenize_fortune_into_array(fortune, &tokens, &tokens_size, &tokens_cap);

	int longest_common = longest_common_substring(tokens, tokens_size, data->tokens, data->tokens_size);
	score *= longest_common;

	free(tokens);
	
	// score /= index_get_fortune_length(data->index, fortune); // length decay
	
	return score;
}

static int _compare_token_score(const void* _a, const void* _b) {
	const hash_t* a = _a, *b = _b;
	return *b - *a;
}

static uint64_t pick_for_input(Index* index, char* input, uint64_t *avoid, uint64_t avoid_size) {
	uint64_t fortune;
	FortuneSet* fs;
	fortune_set_init(&fs);

	hash_t* tokens;
	uint64_t tokens_size, tokens_cap, i;

	tokenize_into_array(input, &tokens, &tokens_size, &tokens_cap);
	qsort(tokens, tokens_size, sizeof(*tokens), _compare_token_score);

	// 10000 = magic constant
	for (i = 0; i < tokens_size && fortune_set_get_size(fs) < 10000; i++) {
		if (!index_contains_word(index, tokens[i])) {
			continue;
		}
		index_for_each_word_fortune(index, tokens[i], _matching_fortune_callback, & (matching_fortune_callback_data) {
			.index = index,
			.fortune_set = fs,
			.hash = tokens[i]
		});
	}

	if (fortune_set_is_empty(fs)) {
		fortune = index_get_random_fortune(index);
	} else {
		fortune_set_adjust_score(fs, _adjust_score, & (adjust_score_data) {
			.index = index,
			.tokens = tokens,
			.tokens_size = tokens_size
		});
		fortune = fortune_set_pick(fs, avoid, avoid_size);
	}

	fortune_set_destroy(&fs);
	return fortune;
}

static void read_fortune(uint64_t fortune, char** _buffer, uint64_t *buffer_cap) {
	fseek(fortunes, fortune, SEEK_SET);
	char line[1024];
	uint64_t i = 0, read;
	char* buffer = *_buffer;
	buffer[0] = '\0';

	while (!feof(fortunes)) {
		if (!(read = fread(line, 1, sizeof(line), fortunes))) {
			if (feof(fortunes)) break;
			error("Error reading!");
			break;
		}
		if (strcmp(line, "%\n") == 0) break;

		while (*buffer_cap < read + i + 1) {
			*buffer_cap *= 2;
			buffer = realloc(buffer, *buffer_cap);
			*_buffer = buffer;
		}
		memcpy(buffer + i, line, read);
		i += read;
	}
	buffer[i] = '\0';
}

static void backlog_push(uint64_t* backlog, int* size, const int cap, const uint64_t item) {
	assert(*size <= cap);
	int i;
	if (*size == cap) {
		for (i = 0; i < *size - 1; i++) {
			backlog[i] = backlog[i + 1];
		}
		backlog[*size - 1] = item;
	} else {
		backlog[*size] = item;
		*size = *size + 1;
	}
}

static int find_similar(Index* index, bool do_echo, bool do_continuous, int continuous_interval) {
	index_load(index, INDEX_FILE);

	uint64_t input_cap = 1024, i = 0;
	char *input = malloc(input_cap);
	int read;

	while (!feof(stdin)) {
		if (i == input_cap) {
			input_cap *= 2;
			input = realloc(input, input_cap);
			if (!input) {
				error("Allocation error!");
				return 0;
			}
		}
		if (!(read = fread(input + i, 1, input_cap - i, stdin))) {
			if (feof(stdin)) break;
			error("Failed to read from stdin!");
			free(input);
			return 0;
		}
		input[i + read] = '\0';
		if (do_echo) {
			puts(input + i);
		}
		i += read;
	}

	if (do_continuous) {
		uint64_t backlog[5]; // Used to avoid cycles.
		int backlog_size = 0;
		uint64_t fortune;
		do {
			fortune = pick_for_input(index, input, backlog, backlog_size);
			backlog_push(backlog, &backlog_size, sizeof(backlog) / sizeof(*backlog), fortune);
			print_fortune(fortune);
			read_fortune(fortune, &input, &input_cap);
			sleep(continuous_interval);
		} while (true);
	} else {
		print_fortune(pick_for_input(index, input, NULL, 0));
	}
	free(input);
	return 1;
}

// TODO: specifikovat soubor?
static void show_usage(const char* program) {
	printf("Usage: %s [--rebuild-index | --continuous]\n", program);
	printf("\t--rebuild-index: rebuild the fortune index\n");
	printf("\t--continuous: \"stream-of-consciousness mode\"\n");
	printf("\t--echo: echo back input plus relevant fortune\n");
}

int main(int argc, char** argv) {
	int f = 0;
	bool do_build_index = false, do_continuous = false, do_echo = false;

	(void) argc; (void) argv;

	srand(time(NULL));
	fortunes = fopen("fortunes", "r");

	if (!fortunes) {
		error("Failed to open fortunes file!");
		return 3;
	}

	Index* index;
	index_init(&index);

	if (argc > 1) {
		if (argc == 2 && strcmp(argv[1], "--rebuild-index") == 0) {
			do_build_index = true;
		} else if (argc == 2 && strcmp(argv[1], "--continuous") == 0) {
			do_continuous = true;
		} else if (argc == 2 && strcmp(argv[1], "--echo") == 0) {
			do_echo = true;
		} else {
			show_usage(argv[0]);
			f = 1;
			goto exit;
		}
	}

	if (do_build_index) {
		if (!build_index(index)) {
			error("Failed to build index.");
			f = 2;
		}
	} else {
		if (!find_similar(index, do_echo, do_continuous, 10)) {
			f = 1;
		}
	}

exit:
	index_destroy(&index); // TODO takeback
	fclose(fortunes);
	return f;
}
