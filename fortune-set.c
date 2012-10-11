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
	uint64_t entriesSize, entriesCapacity;
};

void fortune_set_init(FortuneSet** _this) {
	FortuneSet* this;
	this = malloc(sizeof(*this));
	this->entriesSize = 0;
	this->entriesCapacity = 4;
	this->entries = malloc(this->entriesCapacity * sizeof(*this->entries));
	*_this = this;
}

void fortune_set_destroy(FortuneSet** _this) {
	free(*_this);
}

void fortune_set_add_score(FortuneSet* this, uint64_t fortune, float score) {
	uint64_t i;
	for (i = 0; i < this->entriesSize; i++) {
		Entry* entry = &this->entries[i];
		if (entry->fortune == fortune) {
			entry->score += score;
			break;
		}
	}

	this->entriesSize++;
	while (this->entriesCapacity < this->entriesSize) {
		this->entriesCapacity <<= 1;
		this->entries = realloc(this->entries, this->entriesCapacity * sizeof(*this->entries));
	}
	this->entries[this->entriesSize - 1].fortune = fortune;
	this->entries[this->entriesSize - 1].score = score;
}

int _compareEntries(const void* _a, const void* _b) {
	const Entry* a = _a, *b = _b;
	if (a->score > b->score) return -1;
	if (a->score < b->score) return 1;
	return 0;
}

uint64_t fortune_set_pick(FortuneSet* this, uint64_t avoid) {
	if (fortune_set_is_empty(this)) {
		error("Cannot pick a fortune from an empty set!");
		return -1;
	}

	qsort(this->entries, this->entriesSize, sizeof(*this->entries), _compareEntries);
	assert(this->entries[0].score >= this->entries[1].score);

	uint64_t i, j;
	float scores[10];
	uint64_t fortunes[sizeof(scores) / sizeof(*scores)];
	float min, max;

	for (i = 0, j = 0; j < sizeof(scores) / sizeof(*scores) && i < this->entriesSize; i++) {
		Entry* entry = &this->entries[i];
		if (entry->fortune == avoid) continue;
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
	for (i = 0; i < this->entriesSize; i++) {
		if (rand() % 2 == 0) return this->entries[i].fortune;
	}
	exit(1);
	return -1;
	*/
}

bool fortune_set_is_empty(FortuneSet* this) {
	return this->entriesSize == 0;
}
