#ifndef DICTIONARY_H_INCLUDED
#define DICTIONARY_H_INCLUDED

#include <stdbool.h>
#include "common.h"

typedef struct Dictionary Dictionary;

// Stores HASH -> WORD; COUNT; FORTUNES

void dictionary_init(Dictionary** _dictionary);
void dictionary_destroy(Dictionary** _dictionary);
uint64_t dictionary_get_words_total(Dictionary* dictionary);
void dictionary_get_top_entries(Dictionary* dictionary, hash_t *hashes, int count);
void dictionary_forget_word(Dictionary* this, hash_t hash);
void dictionary_insert_word(Dictionary* dictionary, hash_t hash, uint64_t fortune);
void dictionary_get_entry(Dictionary* dictionary, hash_t hash, int* word_count);
void dictionary_for_each_word_fortune(Dictionary* dictionary, hash_t hash, void (*callback)(void* opaque, uint64_t fortune), void* opaque);
bool dictionary_contains_word(Dictionary* dictionary, hash_t hash);
void dictionary_insert_fortune(Dictionary* dictionary, uint64_t fortune, int length);
uint64_t dictionary_get_random_fortune(Dictionary* dictionary);

int dictionary_save(Dictionary* dictionary, const char* filename);
int dictionary_load(Dictionary* dictionary, const char* filename);

#endif
