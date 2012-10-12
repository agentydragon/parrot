#include "build-index.h"
#include "common.h"
#include "hash.h"
#include "tokenizer.h"

#include <stdio.h>
#include <string.h>

typedef struct {
	Index* index;
	uint64_t fortune;
	int length;
} fortune_token_callback_data;

static void _fortune_token_callback(void* _opaque, const char* word, int length) {
	(void) word; (void) length;
	fortune_token_callback_data* data = _opaque;
	hash_t hash = get_hash(word, length);
	index_insert_word(data->index, hash, data->fortune);
	data->length++;
}

static void load_fortune(Index* index, FILE* file) {
	char buffer[256] = { 0 };

	uint64_t fortune = ftell(file);
	fortune_token_callback_data callback_data = {
		.index = index,
		.fortune = fortune,
		.length = 0
	};
#if DEBUG
	printf("F #%ld: ", fortune);
#endif
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
		for_each_token(buffer, _fortune_token_callback, &callback_data);
	}

	index_insert_fortune(index, fortune, callback_data.length);
}

int build_index(Index* index) {
	printf("Tokenizing fortunes...\n");
	// TODO: more files...

	while (!feof(fortunes)) {
		load_fortune(index, fortunes);
	}

	// Forget top 20 words.
	hash_t hashes[20];
	const int count = sizeof(hashes) / sizeof(*hashes);
	int i, word_count;
	printf("Removing top words...\n");
	index_get_top_entries(index, hashes, count);
	for (i = 0; i < count; i++) {
		index_get_entry(index, hashes[i], &word_count);
		index_forget_word(index, hashes[i]);
	}

	printf("Saving index...\n");
	index_save(index, INDEX_FILE);

	printf("Done.\n");

	return 1;
}
