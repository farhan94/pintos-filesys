#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <stdbool.h>

void syscall_init (void);

bool create(const char *file, unsigned initial_size);
bool remove(const char *file);

#endif /* userprog/syscall.h */
