#ifndef DIR_TOKENIZER_H
#define DIR_TOKENIZER_H

#include <stdbool.h>

void dirtok_init(char const* path);
bool dirtok_next(char* buf);
bool dirtok_hasnext();

void dirtok_test();


#endif
