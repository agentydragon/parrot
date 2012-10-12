#ifndef INDEX_H_INCLUDED
#define INDEX_H_INCLUDED

#include <stdbool.h>
#include "common.h"

typedef struct Index Index;

// Stores HASH -> WORD; COUNT; FORTUNES

void index_init(Index** _index);
void index_destroy(Index** _index);
uint64_t index_get_words_total(Index* index);
void index_get_top_entries(Index* index, hash_t *hashes, int count);
void index_forget_word(Index* this, hash_t hash);
void index_insert_word(Index* index, hash_t hash, uint64_t fortune);
void index_get_entry(Index* index, hash_t hash, int* word_count);
void index_for_each_word_fortune(Index* index, hash_t hash, void (*callback)(void* opaque, uint64_t fortune), void* opaque);
bool index_contains_word(Index* index, hash_t hash);
void index_insert_fortune(Index* index, uint64_t fortune, int length);
uint64_t index_get_random_fortune(Index* index);
uint64_t index_get_fortune_length(Index* this, uint64_t fortune);

int index_save(Index* index, const char* filename);
int index_load(Index* index, const char* filename);

#endif
