#include "index.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct {
	hash_t hash;
	int count;

	uint64_t *fortunes;
	uint64_t fortunes_size, fortunes_cap; // TODO
} Entry;

typedef struct {
	uint64_t fortune;
	int length;
} Fortune;

struct Index {
	Entry* entries;
	uint64_t entries_size, entries_cap;

	Fortune* fortunes;
	uint64_t fortunes_size, fortunes_cap;

	uint64_t words_total;
};

static void _realloc_entries(Index* this) {
	while (this->entries_cap < this->entries_size) {
		if (!this->entries_cap) {
			this->entries_cap = 64;
		} else {
			this->entries_cap <<= 1;
		}
		this->entries = realloc(this->entries, this->entries_cap * sizeof(*this->entries));
	}
}

static void _realloc_fortunes(Index* this) {
	while (this->fortunes_cap < this->fortunes_size) {
		if (!this->fortunes_cap) {
			this->fortunes_cap = 64;
		} else {
			this->fortunes_cap <<= 1;
		}
		this->fortunes = realloc(this->fortunes, this->fortunes_cap * sizeof(*this->fortunes));
	}
}

static void _realloc_entry_fortunes(Entry* entry) {
	while (entry->fortunes_cap < entry->fortunes_size) {
		if (!entry->fortunes_cap) {
			entry->fortunes_cap = 1;
		} else {
			entry->fortunes_cap <<= 1;
		}
	}
	entry->fortunes = realloc(entry->fortunes, entry->fortunes_cap * sizeof(*entry->fortunes));
}

void index_init(Index** _this) {
	Index* index = malloc(sizeof(Index));
	index->entries = NULL;
	index->entries_size = index->entries_cap = 0;
	
	index->fortunes = NULL;
	index->fortunes_size = index->fortunes_cap = 0;

	index->words_total = 0;
	// TODO: chci to rychlejsi...
	
	*_this = index;
}

void index_destroy(Index** _index) {
	uint64_t i;
	Index* this = *_index;

	for (i = 0; i < this->entries_size; i++) {
		free(this->entries[i].fortunes);
	}
	free(this->entries);

	free(this->fortunes);

	free(this);
	*_index = NULL;
}

void index_insert_word(Index* this, hash_t hash, uint64_t fortune) {
	uint64_t i;
	this->words_total++;

	for (i = 0; i < this->entries_size; i++) {
		Entry* entry = &this->entries[i];
		if (entry->hash == hash) {
			entry->count++;

			uint64_t x;
			for (x = 0; x < entry->fortunes_size; x++) {
				if (entry->fortunes[x] == fortune) return;
			}
			entry->fortunes_size++;
			_realloc_entry_fortunes(entry);
			entry->fortunes[entry->fortunes_size - 1] = fortune;

			return;
		}
	}

	this->entries_size++;
	_realloc_entries(this);

	Entry* entry = &this->entries[this->entries_size - 1];
	entry->hash = hash;
	entry->count = 1;
	entry->fortunes_size = 1;
	entry->fortunes_cap = 8;
	entry->fortunes = malloc(sizeof(*entry->fortunes) * entry->fortunes_cap);
	entry->fortunes[0] = fortune;
}

void index_get_entry(Index* this, hash_t hash, int* word_count) {
	uint64_t i;

	for (i = 0; i < this->entries_size; i++) {
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
	qsort(this->entries, this->entries_size, sizeof(Entry), _compareEntries);

	for (i = 0; i < count; i++) {
		hashes[i] = this->entries[i].hash;
	}
}

void index_forget_word(Index* this, hash_t hash) {
	uint64_t i;
	for (i = 0; i < this->entries_size; i++) {
		if (this->entries[i].hash == hash) break;
	}
	free(this->entries[i].fortunes);
	memmove(&this->entries[i], &this->entries[i + 1], sizeof(Entry) * this->entries_size - i - 1);
	this->entries_size--;
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
	for (i = 0; i < this->entries_size; i++) {
		if (this->entries[i].hash == hash) {
			for (j = 0; j < this->entries[i].fortunes_size; j++) {
				callback(opaque, this->entries[i].fortunes[j]);
			}
			return;
		}
	}
}

bool index_contains_word(Index* this, hash_t hash) {
	uint64_t i;
	for (i = 0; i < this->entries_size; i++) {
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
		fwrite(&this->entries_size, sizeof(this->entries_size), 1, f) != 1) {
		error("Failed to write to index file.");
		goto close_and_die;
	}
	
	uint64_t i;
	for (i = 0; i < this->entries_size; i++) {
		Entry* entry = &this->entries[i];
		if (fwrite(&entry->hash, sizeof(entry->hash), 1, f) != 1 ||
			fwrite(&entry->count, sizeof(entry->count), 1, f) != 1 ||
			fwrite(&entry->fortunes_size, sizeof(entry->fortunes_size), 1, f) != 1 ||
			fwrite(entry->fortunes, sizeof(*entry->fortunes), entry->fortunes_size, f) != entry->fortunes_size) {
			error("Failed to write token #%ld into index file.", i);
			goto close_and_die;
		}
	}

	if (fwrite(&this->fortunes_size, sizeof(this->fortunes_size), 1, f) != 1) {
		error("Failed to write fortunes_size to index file.");
		goto close_and_die;
	}

	for (i = 0; i < this->fortunes_size; i++) {
		if (fwrite(&this->fortunes[i].fortune, sizeof(this->fortunes[i].fortune), 1, f) != 1 ||
			fwrite(&this->fortunes[i].length, sizeof(this->fortunes[i].length), 1, f) != 1) {
			error("Failed to write fortune to index file");
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
		fread(&this->entries_size, sizeof(this->entries_size), 1, f) != 1) {
		error("Failed to read data from index file.");
		goto close_and_die;
	}
	_realloc_entries(this);

	uint64_t i;
	for (i = 0; i < this->entries_size; i++) {
		Entry* entry = &this->entries[i];
		if (fread(&entry->hash, sizeof(entry->hash), 1, f) != 1 ||
			fread(&entry->count, sizeof(entry->count), 1, f) != 1 ||
			fread(&entry->fortunes_size, sizeof(entry->fortunes_size), 1, f) != 1) {
			error("Failed to read token #%ld data from index file.", i);
			goto close_and_die;
		}
		entry->fortunes = NULL;
		entry->fortunes_cap = 0;
		_realloc_entry_fortunes(entry);
		if (fread(entry->fortunes, sizeof(*entry->fortunes), entry->fortunes_size, f) != entry->fortunes_size) {
			error("Failed to read token #%ld from index file.", i); 
			goto close_and_die;
		}
	}

	if (fread(&this->fortunes_size, sizeof(this->fortunes_size), 1, f) != 1) {
		error("Failed to read fortunes_size from index file.");
		goto close_and_die;
	}
	_realloc_fortunes(this);

	for (i = 0; i < this->fortunes_size; i++) {
		if (fread(&this->fortunes[i].fortune, sizeof(this->fortunes[i].fortune), 1, f) != 1 ||
			fread(&this->fortunes[i].length, sizeof(this->fortunes[i].length), 1, f) != 1) {
			error("Failed to read fortune from index file");
			goto close_and_die;
		}
	}

	fclose(f);
	return 1;

close_and_die:
	fclose(f);
	return 0;
}

void index_insert_fortune(Index* this, uint64_t fortune, int length) {
	this->fortunes_size++;
	_realloc_fortunes(this);

	this->fortunes[this->fortunes_size - 1].fortune = fortune;
	this->fortunes[this->fortunes_size - 1].length = length;
}

uint64_t index_get_random_fortune(Index* this) {
	return this->fortunes[rand() % this->fortunes_size].fortune;
}
