
#include "dir-tokenizer.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "filesys/directory.h"

char dirname_full[DIRNAME_MAX];
char dirname_cur[NAME_MAX];
char* dir_ptr;

void dirtok_init(char const* dirname) {
    strlcpy(dirname_full, dirname, sizeof(dirname_full));
    // printf("Full dirname: %s\n", dirname_full);
    dir_ptr = dirname_full;
}

/*
  * Writes the next token into the buffer buf. Returns true if there was a next token, false if not.
  */
bool dirtok_next(char* buf) {
    // return false;
    if (*dir_ptr == '\0') {
        return false;
    }
    while (*dir_ptr == '/') {    // skip all consecutive "/"
        ++dir_ptr;
    }
    char* dir_ptr_end = dir_ptr;
    while (*dir_ptr_end != '/' && *dir_ptr_end != '\0') {   // find the end of the current directory name
        ++dir_ptr_end;
    }
    uint32_t len = dir_ptr_end - dir_ptr;
    if (len == 0) {
        return false;
    }
    strlcpy(buf, dir_ptr, len + 1);
    // buf[len] = '\0';
    /* Set pointer to the next '/' */
    dir_ptr = dir_ptr_end;
    return true;
    
}

bool dirtok_hasnext() {
    if (*dir_ptr == '\0') {
        return false;
    }
    while (*dir_ptr == '/') {
        ++dir_ptr;
    }
    if (*dir_ptr == '\0') {
        return false;
    }
    return true;
}

void dirtok_test() {
    dirtok_init("adsaf/btase/cdd//d123/////edfsa/");
    char dirname[NAME_MAX];
    while (dirtok_next(dirname)) {
        printf("%s\n", dirname);
    }
}
