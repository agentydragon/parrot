#include "index.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct {
	hash_t hash;
	int count;

	uint64_t *fortunes;
	uint64_t fortunesCount, fortunesCapacity; // TODO
} Entry;

struct Index {
	Entry* entries;
	uint64_t entryCount, entryCapacity;

	uint64_t words_total;
};

static void _realloc_entries(Index* this) {
	while (this->entryCapacity < this->entryCount) {
		if (!this->entryCapacity) {
			this->entryCapacity = 64;
		} else {
			this->entryCapacity <<= 1;
		}
		this->entries = realloc(this->entries, this->entryCapacity * sizeof(*this->entries));
	}
}

static void _realloc_fortunes(Entry* entry) {
	while (entry->fortunesCapacity < entry->fortunesCount) {
		if (!entry->fortunesCapacity) {
			entry->fortunesCapacity = 1;
		} else {
			entry->fortunesCapacity <<= 1;
		}
	}
	entry->fortunes = realloc(entry->fortunes, entry->fortunesCapacity * sizeof(*entry->fortunes));
}

void index_init(Index** _this) {
	Index* index = malloc(sizeof(Index));
	index->entries = NULL;
	index->entryCount = index->entryCapacity = 0;

	index->words_total = 0;
	// TODO: chci to rychlejsi...
	
	*_this = index;
}

void index_destroy(Index** _index) {
	free(*_index);
}

void index_insert_word(Index* this, hash_t hash, uint64_t fortune) {
	uint64_t i;
	this->words_total++;

	for (i = 0; i < this->entryCount; i++) {
		Entry* entry = &this->entries[i];
		if (entry->hash == hash) {
			entry->count++;

			uint64_t x;
			for (x = 0; x < entry->fortunesCount; x++) {
				if (entry->fortunes[x] == fortune) return;
			}
			entry->fortunesCount++;
			_realloc_fortunes(entry);
			entry->fortunes[entry->fortunesCount - 1] = fortune;

			// printf("#%X += %ld\n", hash, fortune);

			return;
		}
	}

	this->entryCount++;
	_realloc_entries(this);

	Entry* entry = &this->entries[this->entryCount - 1];
	entry->hash = hash;
	entry->count = 1;
	entry->fortunesCount = 1;
	entry->fortunesCapacity = 8;
	entry->fortunes = malloc(sizeof(*entry->fortunes) * entry->fortunesCapacity);
	entry->fortunes[0] = fortune;
}

void index_get_entry(Index* this, hash_t hash, /*char* word, int *length, int capacity, */ int* word_count) {
	uint64_t i;

	for (i = 0; i < this->entryCount; i++) {
		Entry* entry = &this->entries[i];
		if (entry->hash == hash) {
			*word_count = entry->count;
			return;
		}
	}

	printf("Not found!");
	exit(1);
	// TODO
}

static int _compareEntries(const void* _a, const void* _b) {
	const Entry* a = _a, *b = _b;
	return b->count - a->count;
}

void index_get_top_entries(Index* this, hash_t *hashes, int count) {
	int i;
	qsort(this->entries, this->entryCount, sizeof(Entry), _compareEntries);

	for (i = 0; i < count; i++) {
		hashes[i] = this->entries[i].hash;
	}
}

void index_forget_word(Index* this, hash_t hash) {
	uint64_t i;
	for (i = 0; i < this->entryCount; i++) {
		if (this->entries[i].hash == hash) break;
	}
	memmove(&this->entries[i], &this->entries[i + 1], sizeof(Entry) * this->entryCount - i - 1);
	this->entryCount--;
}

uint64_t index_get_words_total(Index* this) {
	return this->words_total;
}

int _compareFortunes(const void* _a, const void* _b) {
	const uint64_t *a = _a, *b = _b;
	if (*a > *b) return 1;
	if (*a < *b) return -1;
	return 0;
}

void index_for_each_word_fortune(Index* this, hash_t hash, void (*callback)(void* opaque, uint64_t fortune), void* opaque) {
	uint64_t i, j;
	for (i = 0; i < this->entryCount; i++) {
		if (this->entries[i].hash == hash) {
			for (j = 0; j < this->entries[i].fortunesCount; j++) {
				callback(opaque, this->entries[i].fortunes[j]);
			}
			return;
		}
	}
}

bool index_contains_word(Index* this, hash_t hash) {
	uint64_t i;
	for (i = 0; i < this->entryCount; i++) {
		if (this->entries[i].hash == hash) {
			return true;
		}
	}
	return false;
}

int index_save(Index* this, const char* filename) {
	FILE* f = fopen(filename, "w");
	if (!f) {
		error("Failed to open index file %s for writing.", filename);
		return 0;
	}
	if (fwrite(&this->words_total, sizeof(this->words_total), 1, f) != 1 ||
		fwrite(&this->entryCount, sizeof(this->entryCount), 1, f) != 1) {
		error("Failed to write to index file.");
		goto close_and_die;
	}
	
	uint64_t i;
	for (i = 0; i < this->entryCount; i++) {
		Entry* entry = &this->entries[i];
		if (fwrite(&entry->hash, sizeof(entry->hash), 1, f) != 1 ||
			fwrite(&entry->count, sizeof(entry->count), 1, f) != 1 ||
			fwrite(&entry->fortunesCount, sizeof(entry->fortunesCount), 1, f) != 1 ||
			fwrite(entry->fortunes, sizeof(*entry->fortunes), entry->fortunesCount, f) != entry->fortunesCount) {
			error("Failed to write token #%ld into index file.", i);
			goto close_and_die;
		}
	}

	fclose(f);
	return 1;

close_and_die:
	fclose(f);
	return 0;
}

int index_load(Index* this, const char* filename) {
	FILE* f = fopen(filename, "r");
	if (!f) {
		error("Failed to open index file %s for reading. Build it if it doesn't exist.", filename);
		return 0;
	}

	if (fread(&this->words_total, sizeof(this->words_total), 1, f) != 1 ||
		fread(&this->entryCount, sizeof(this->entryCount), 1, f) != 1) {
		error("Failed to read data from index file.");
		goto close_and_die;
	}
	_realloc_entries(this);

	uint64_t i;
	for (i = 0; i < this->entryCount; i++) {
		Entry* entry = &this->entries[i];
		if (fread(&entry->hash, sizeof(entry->hash), 1, f) != 1 ||
			fread(&entry->count, sizeof(entry->count), 1, f) != 1 ||
			fread(&entry->fortunesCount, sizeof(entry->fortunesCount), 1, f) != 1) {
			error("Failed to read token #%ld data from index file.", i);
		}
		entry->fortunes = NULL;
		_realloc_fortunes(entry);
		if (fread(entry->fortunes, sizeof(*entry->fortunes), entry->fortunesCount, f) != entry->fortunesCount) {
			error("Failed to read token #%ld fortune list from index file.", i); 
		}
	}

	fclose(f);
	return 1;

close_and_die:
	fclose(f);
	return 0;
}

void index_insert_fortune(Index* this, uint64_t fortune, int length) {
	error("TODO: insert_fortune");
}

uint64_t index_get_random_fortune(Index* this) {
	error("TODO: get_random_fortune");
	return 1;
}
