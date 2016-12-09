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
  // printf("creating file/dir %s\n", name);
  if (strlen(name) > NAME_MAX) {
    return false;
  }
  block_sector_t inode_sector = 0;
  /* Check if name is absolute or relative */
  char path[DIRNAME_MAX];
  dirtok_get_abspath_updir(name, path);
  struct dir* dir = dir_open_path(path);

  char filename[NAME_MAX];
  dirtok_get_filename(name, filename);

  bool success;
  if (is_dir) {
    // printf("making dir %s in %s\n", filename, path);
    success = dir != NULL
                        && free_map_allocate(1, &inode_sector)
                        && dir_create(inode_sector, 0)
                        && dir_add(dir, filename, inode_sector);
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
    // printf("Successfully created file/dir %s\n", name);
  }
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
  //printf("\n\n\n@@@HERE\n\n\n");
  char path[DIRNAME_MAX];
  dirtok_get_abspath_updir(name, path);
  struct dir *dir = dir_open_path(path);
  struct inode *inode = NULL;

  char filename[NAME_MAX];
  dirtok_get_filename(name, filename);

  bool success;
  if (dir != NULL)
    success = dir_lookup (dir, filename, &inode);
  // printf("Dir lookup succeeded in open: %d\n", success);
  dir_close (dir);

  return file_open (inode);
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name) 
{
  char path[DIRNAME_MAX];
  dirtok_get_abspath(name, path);
  if (*path == '/' && *(path + 1) == '\0') { // can't remove root directory'
    // printf("can't remove root\n'");
    return false;
  }
  dirtok_get_abspath_updir(name, path);
  struct dir* dir = dir_open_path(path);

  char filename[NAME_MAX];
  dirtok_get_filename(name, filename);
  // printf("Removing %s from %s\n", filename, path);
  bool success = dir != NULL && dir_remove (dir, filename);
  dir_close (dir); 
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
