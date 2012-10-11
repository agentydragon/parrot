#ifndef FORTUNE_SET_H_INCLUDED
#define FORTUNE_SET_H_INCLUDED

#include <stdint.h>

typedef struct FortuneSet FortuneSet;

void fortune_set_init(FortuneSet** _this);
void fortune_set_destroy(FortuneSet** _this);
void fortune_set_add_score(FortuneSet* this, uint64_t fortune, float score);
uint64_t fortune_set_pick(FortuneSet* this, uint64_t avoid);

#endif