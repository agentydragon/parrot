#ifndef TOKENIZER_H_INCLUDED
#define TOKENIZER_H_INCLUDED

void for_each_token(char* line, void* opaque, void (*callback)(void* opaque, const char* word, int length));

#endif
