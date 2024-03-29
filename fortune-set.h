#ifndef FORTUNE_SET_H_INCLUDED
#define FORTUNE_SET_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>

typedef struct FortuneSet FortuneSet;

void fortune_set_init(FortuneSet** _this);
void fortune_set_destroy(FortuneSet** _this);
void fortune_set_add_score(FortuneSet* this, uint64_t fortune, float score);
uint64_t fortune_set_pick(FortuneSet* this, uint64_t *avoid, uint64_t avoid_size);
void fortune_set_adjust_score(FortuneSet* this, float (*get_new_score)(void* opaque, uint64_t fortune, float former_score), void* opaque);
bool fortune_set_is_empty(FortuneSet* this);
uint64_t fortune_set_get_size(FortuneSet* this);

#endif
