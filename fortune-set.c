#include "fortune-set.h"
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

typedef struct {
	uint64_t fortune;
	float score;
} Entry;

struct FortuneSet {
	Entry* entries;
	uint64_t entries_size, entries_cap;
};

void fortune_set_init(FortuneSet** _this) {
	FortuneSet* this;
	this = malloc(sizeof(*this));
	this->entries_size = 0;
	this->entries_cap = 4;
	this->entries = malloc(this->entries_cap * sizeof(*this->entries));
	*_this = this;
}

void fortune_set_destroy(FortuneSet** _this) {
	FortuneSet* this = *_this;
	free(this->entries);
	free(this);
	*_this = NULL;
}

void fortune_set_add_score(FortuneSet* this, uint64_t fortune, float score) {
	uint64_t i;
	for (i = 0; i < this->entries_size; i++) {
		Entry* entry = &this->entries[i];
		if (entry->fortune == fortune) {
			entry->score += score;
			return;
		}
	}

	this->entries_size++;
	while (this->entries_cap < this->entries_size) {
		this->entries_cap <<= 1;
		this->entries = realloc(this->entries, this->entries_cap * sizeof(*this->entries));
	}
	this->entries[this->entries_size - 1].fortune = fortune;
	this->entries[this->entries_size - 1].score = score;
}

void fortune_set_adjust_score(FortuneSet* this, float (*get_new_score)(void* opaque, uint64_t fortune, float former_score), void* opaque) {
	uint64_t i;
	for (i = 0; i < this->entries_size; i++) {
		Entry* entry = &this->entries[i];
		entry->score = get_new_score(opaque, entry->fortune, entry->score);
	}
}

static int _compare_entries(const void* _a, const void* _b) {
	const Entry* a = _a, *b = _b;
	if (a->score > b->score) return -1;
	if (a->score < b->score) return 1;
	return 0;
}

static bool _array_contains(uint64_t* array, uint64_t size, uint64_t needle) {
	uint64_t i = 0;
	for (i = 0; i < size; i++) {
		if (array[i] == needle) return true;
	}
	return false;
}

uint64_t fortune_set_pick(FortuneSet* this, uint64_t *avoid, uint64_t avoid_size) {
	uint64_t i, j;
	if (fortune_set_is_empty(this)) {
		error("Cannot pick a fortune from an empty set!");
		return -1;
	}

	qsort(this->entries, this->entries_size, sizeof(*this->entries), _compare_entries);
	assert(this->entries_size < 2 || this->entries[0].score >= this->entries[1].score);

	// Find 10 fortunes with the best score, but outside the array of avoided fortunes.
	float scores[10];
	const int N = sizeof(scores) / sizeof(*scores);
	uint64_t fortunes[N];
	float min, max;

	for (i = 0, j = 0; j < N && i < this->entries_size; i++) {
		Entry* entry = &this->entries[i];
		if (_array_contains(avoid, avoid_size, entry->fortune)) {
			continue;
		}
		scores[j] = entry->score;
		fortunes[j] = entry->fortune;
		if (j == 0 || min > scores[j]) min = scores[j];
		if (j == 0 || max < scores[j]) max = scores[j];
		j++;
	}

#if DEBUG
	printf("min=%f max=%f\n", min, max);
#endif

	float total;
	for (i = 0, total = 0; i < j; i++) {
		if (max != min) {
			scores[i] = (scores[i] - min) / (max - min);
		}
#if DEBUG
		printf("score[%ld] (%ld) = %f\n", i, fortunes[i], scores[i]);
#endif
		total += scores[i];
	}
#if DEBUG
	printf("total = %f\n", total);
#endif

	float r = total * ((float)rand() / (float)RAND_MAX);
#if DEBUG
	printf("rand = %f\n", r);
#endif
	for (i = 0; i < j; i++) {
		r -= scores[i];
		if (r <= 0) return fortunes[i];
	}

	error("Failed to pick a fortune!");
	return -1;

	/*
	for (i = 0; i < this->entries_size; i++) {
		if (rand() % 2 == 0) return this->entries[i].fortune;
	}
	exit(1);
	return -1;
	*/
}

bool fortune_set_is_empty(FortuneSet* this) {
	return this->entries_size == 0;
}
