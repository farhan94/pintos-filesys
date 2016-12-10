
#include "dir-tokenizer.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "filesys/directory.h"
#include "threads/thread.h"

char* dirname_full;
char dirname_cur[NAME_MAX + 1];
char* dir_ptr;

void dirtok_init(char const* dirname) {
    dirname_full = (char*)malloc(sizeof(char) * (DIRNAME_MAX + 1));
    strlcpy(dirname_full, dirname, strlen(dirname) + 1);
    // printf("Full dirname: %s\n", dirname_full);
    dir_ptr = dirname_full;
}

/*
  * Writes the next token into the buffer buf. Returns true if there was a next token, false if not.
  */
bool dirtok_next(char* buf) {
    // return false;
    if (*dir_ptr == '\0') {
        free(dirname_full);
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
        free(dirname_full);
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
    char dirname[DIRNAME_MAX + 1];
    while (dirtok_next(dirname)) {
        printf("%s\n", dirname);
    }
}


/*
  * Get full pathname of the directory immediately before the specified new file/directory.
  * For example, if we want to create a new file or directory at path "/a/b/c", this function would return "/a/b"
  */
void dirtok_get_abspath_updir(char const* pathname, char* buf) {
    /* First get the full pathname */
    dirtok_get_abspath(pathname, buf);
    size_t len = strlen(buf);
    char* end_ptr = buf + len;  // take us to the sentinel '\0'
    /* Backtrack until we hit the first '/' char and rewrite the character right after it as the sentinel '\0' */
    while (*end_ptr != '/') {
        --end_ptr;
    }
    *(end_ptr + 1) = '\0';
    // printf("Absolute pathname of upper directory is %s\n", buf);
}



/* Get full pathname of the directory/file. */
void dirtok_get_abspath(char const* pathname, char* buf) {
    // printf("Getting abspath of %s\n", pathname);
    // printf("current thread's dir is %s\n", thread_current()->cur_dir);
    if (strlen(pathname) >= DIRNAME_MAX) {
        printf("dir-tokenizer (dirtok_get_abspath): overflow on path name %s.\n", pathname);
        return false;
    }
    if (*pathname == '/') { // absolute
        strlcpy(buf, pathname, strlen(pathname) + 1);
    }
    else {                        // relative
        // printf("%d\n", buf);
        strlcpy(buf, thread_current()->cur_dir, strlen(thread_current()->cur_dir) + 1);
        size_t len = strlen(buf);
        // printf("len is %d\n", len);
        /* Check if end of buffer ends with a '/' character or not, and if not then write it. */
        if (buf[len - 1] != '/') {
            buf[len] = '/';
            buf[len + 1] = '\0';
        }

        /* Now concatenate the rest of the pathname. */
        if (strlcat(buf, pathname, strlen(buf) + strlen(pathname) + 1) >= DIRNAME_MAX) {
            printf("dir-tokenizer (dirtok_get_abspath): overflow on path name %s/%s.\n", buf, pathname);
            return false;
        }
    }
    // printf("Absolute pathname of file/path is %s\n", buf);
}

/* Get the filename of the directory/file. For example, given the path "/a/b/c", this function would return "c" */
void dirtok_get_filename(char const* pathname, char* buf) {
    // printf("Getting filename of %s\n", pathname);
    dirtok_get_abspath(pathname, buf);
    char* ptr = pathname + strlen(pathname);    // take us to the '\0' sentinel
    while (*ptr != '/' && ptr != pathname) {
        --ptr;
    }
    if (*ptr == '/') {
        ++ptr;
    }
    strlcpy(buf, ptr, strlen(ptr) + 1);
    // printf("Filename is %s\n", buf);
}
