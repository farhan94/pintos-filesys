#ifndef DIR_TOKENIZER_H
#define DIR_TOKENIZER_H

#include <stdbool.h>

void dirtok_init(char const* path);
bool dirtok_next(char* buf);
bool dirtok_hasnext();

/*
  * Get full pathname of the directory immediately before the specified new file/directory.
  * For example, if we want to create a new file or directory at path "/a/b/c", this function would return "/a/b"
  */
void dirtok_get_abspath_updir(char const* pathname, char* buf);

/* Get full pathname of the directory/file. */
void dirtok_get_abspath(char const* pathname, char* buf);

/* Get the filename of the directory/file. For example, given the path "/a/b/c", this function would return "c" */
void dirtok_get_filename(char const* pathname, char* buf);

void dirtok_test();


#endif
