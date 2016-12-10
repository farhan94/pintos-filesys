#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "filesys/dir-tokenizer.h"
#include "threads/thread.h"

/* Partition that contains the file system. */
struct block *fs_device;

static void do_format (void);

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format) 
{
  fs_device = block_get_role (BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC ("No file system device found, can't initialize file system.");

  inode_init ();
  free_map_init ();

  if (format) 
    do_format ();

  free_map_open ();
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void) 
{
  free_map_close ();
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create (const char *name, off_t initial_size, bool is_dir) 
{
  dirtok_init(name);
  char name_check[DIRNAME_MAX];
  while (dirtok_next(name_check)) {
    if (strlen(name_check) > NAME_MAX) {
      return false;
    }
  }
  // printf("filesys (filesys_create): creating file/dir %s\n", name);
  block_sector_t inode_sector = 0;
  /* Check if name is absolute or relative */
  // char* path = (char*)malloc(sizeof(char) * (DIRNAME_MAX + 1));
  char path[DIRNAME_MAX + 1];
  dirtok_get_abspath_updir(name, path);
  struct dir* dir = dir_open_path(path);
  if (!dir) {
    // free(path);
    return false;
  }

  char filename[NAME_MAX + 1];
  dirtok_get_filename(name, filename);

  if (strlen(filename) > NAME_MAX) {
    // free(path);
    return false;
  }

  bool success;
  if (is_dir) {
    // printf("making dir %s in %s\n", filename, path);
    success = dir != NULL
                        && free_map_allocate(1, &inode_sector)
                        && dir_create(inode_sector, 0)
                        && dir_add(dir, filename, inode_sector);
    /* Add the "." and ".." hard links */
    block_sector_t parent_inode_sector = dir->inode->sector;
    dirtok_get_abspath(name, path);
    dir = dir_open_path(path);
    success = success
                        && dir_add(dir, ".", inode_sector)
                        && dir_add(dir, "..", parent_inode_sector);
    int t;
  }
  else {
    success = dir != NULL
                      && free_map_allocate (1, &inode_sector)
                      && inode_create (inode_sector, initial_size, false)
                      && dir_add (dir, filename, inode_sector);
  }
  
  if (!success && inode_sector != 0) 
    free_map_release (inode_sector, 1);
  dir_close (dir);

  if (success) {
    if (is_dir) {
      // printf("filesys (filesys_create) - successfully created dir %s\n", path);
    }
    else {
      // printf("filesys (filesys_create) - successfully created file %s%s\n", path, filename);
    }
  }
  // free(path);
  return success;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *name)
{
  if (*name == '/' && *(name + 1) == '\0') {  // if root, just return the root inode
    struct dir* dir = dir_open_root();
    return file_open(dir->inode);
  }
  //printf("\n\n\n@@@HERE\n\n\n");
  // char* path = (char*)malloc(sizeof(char) * (DIRNAME_MAX + 1));
  char path[DIRNAME_MAX + 1];
  dirtok_get_abspath_updir(name, path);
  // printf("opening file/dir %s\n", path);
  struct dir *dir = dir_open_path(path);
  struct inode *inode = NULL;

  char filename[NAME_MAX + 1];
  dirtok_get_filename(name, filename);

  bool success;
  if (dir != NULL) {
    success = dir_lookup (dir, filename, &inode);
  }

  // printf("Dir lookup succeeded in open: %d\n", success);
  dir_close (dir);

  // free(path);
  return file_open (inode);
}

struct inode*
filesys_open_inode(char const* name) {
  if (*name == '/' && *(name + 1) == '\0') {  // if root, just return the root inode
    struct dir* dir = dir_open_root();
    return dir->inode;
  }
  // char* path = (char*)malloc(sizeof(char) * (DIRNAME_MAX + 1));
  char path[DIRNAME_MAX + 1];
  dirtok_get_abspath_updir(name, path);
  // printf("path is %s\n", path);
  struct dir* dir = dir_open_path(path);
  // printf("path is now %s\n", path);
  if (!dir) {
    // printf("filesys (filesys_open_inode): parent dir does not exist.\n");
    // free(path);
    return NULL;
  }

  struct inode* inode = NULL;

  char filename[NAME_MAX + 1];
  dirtok_get_filename(name, filename);

  // printf("the path is %s\n", path - 1);
  // printf("filesys (filesys_open_inode): opening file/dir %s from dir %s\n", filename, path);

  if (!dir_lookup(dir, filename, &inode)) {
    // printf("filesys (filesys_open_inode): failed to find the file: %s in the directory %s\n", filename, path);
    dir_close(dir);
    // free(path);
    return NULL;
  }

  dir_close(dir);
  // free(path);
  return inode;
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name) 
{
  // char* path = (char*)malloc(sizeof(char) * (DIRNAME_MAX + 1));
  char path[DIRNAME_MAX + 1];
  dirtok_get_abspath(name, path);
  if (*path == '/' && *(path + 1) == '\0') { // can't remove root directory'
    // printf("can't remove root\n'");
    // free(path);
    return false;
  }
  dirtok_get_abspath_updir(name, path);
  // printf("filesys (filesys_remove): file/dir's parent dir is %s\n", path);
  struct dir* dir = dir_open_path(path);
 
  if (!dir) {
    // printf("filesys (filesys_remove): dir does not exist.\n");
    // free(path);
    return false;
  }

  char filename[NAME_MAX + 1];
  dirtok_get_filename(name, filename);
  // printf("Removing %s from %s\n", filename, path);

  bool success = dir != NULL && dir_remove (dir, filename);
  dir_close (dir); 
  // free(path);
  // printf("done removing\n");
  return success;
}

/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_create (ROOT_DIR_SECTOR, 16))
    PANIC ("root directory creation failed");
  free_map_close ();
  printf ("done.\n");
}
