#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44
#define DIRECT_BLOCKS 123
#define INDIRECT_BLOCKS 128
/* On-disk inode.
   Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk
  {
    
    off_t length;                       /* File size in bytes. */
    unsigned magic;                     /* Magic number. */
  //  uint32_t unused[125];               /* Not used. */
    bool is_directory;
    block_sector_t direct[123]; //the direct block
    block_sector_t indirect;  //indirect block->contains 127
    block_sector_t doubly_indirect;  //doubly indirect-> each indirect element has one indrect block

  };

struct indirect_block_sec{
  block_sector_t direct[128];
};
/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, BLOCK_SECTOR_SIZE);
}

/* In-memory inode. */
struct inode 
  {
    struct list_elem elem;              /* Element in inode list. */
    block_sector_t sector;              /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
    struct inode_disk data;             /* Inode content. */
  };

/*allocates space in the inode stuff */
bool inode_alloc(struct inode_disk *disk_inode, off_t length){
  static char null_stuff[BLOCK_SECTOR_SIZE];


  if (length < 0){
    return false;
  }
  off_t sectors = bytes_to_sectors(length);
  if(sectors <= DIRECT_BLOCKS){
     //printf("\n\n@@@@\n\n");
    for(int k = 0; k < sectors; k+=1){
    //  printf("\n\nSTART\n\n\n");
     //  printf("\n\n\%d\n\n", disk_inode->direct[k]);
      if(disk_inode->direct[k] == 0){
        if(!free_map_allocate(1, &disk_inode->direct[k])){
          return false;
        }
       block_write(fs_device, disk_inode->direct[k], null_stuff);
      }
      //printf("\n\n\nEND\n\n\n");
    }
    //printf("\n\n\nTRUE\n\n\n");
    return true;
  }
  else{
    for(int k = 0; k < DIRECT_BLOCKS; k+=1){
      if(disk_inode->direct[k] == 0){
        if(!free_map_allocate(1, &disk_inode->direct[k])){
          return false;
        }
        block_write(fs_device, disk_inode->direct[k], null_stuff);
      }
    }
    sectors-=DIRECT_BLOCKS;
    off_t num_alloc;
    if(sectors <= INDIRECT_BLOCKS){
      num_alloc = sectors;
    }
    else{
      num_alloc = INDIRECT_BLOCKS;
    }
   
    if(disk_inode->indirect == NULL || disk_inode->indirect == 0){
      if(!free_map_allocate(1, &disk_inode->indirect)){
        return false;
      }
      block_write(fs_device, disk_inode->indirect, null_stuff);
    }

    struct indirect_block_sec indirect_sector;
    block_read(fs_device, disk_inode->indirect, &indirect_sector);
    for(int k = 0; k <= num_alloc; k++){
      if(indirect_sector.direct[k] == NULL || indirect_sector.direct[k] == 0){
          if(!free_map_allocate(1, &indirect_sector.direct[k])){
            return false;
          } //ADD THE IF
        }
      //disk_inode->indirect_block_sec += 1; //Should we be incrementing this???
    }
    block_write(fs_device, disk_inode->indirect, &indirect_sector);
    return true;
    // sectors -= num_alloc;
    // if(sectors <= 0){
    //   return true;
    // }
    // else{
    //   return false;
    // }
  }
}
/* deallocates the space in inode stuff */
bool inode_dealloc(struct inode *inode){
  struct inode_disk disk_inode = inode->data;
  off_t length = disk_inode.length;
  size_t sectors = length/BLOCK_SECTOR_SIZE;
  size_t num_dealloc;
  if(sectors <= DIRECT_BLOCKS){
    num_dealloc = sectors;
  }
  else{
    num_dealloc = DIRECT_BLOCKS;
  }
  for (int k = 0; k < num_dealloc; k = 0){
    if(disk_inode.direct[k] != 0){
      free_map_release(disk_inode.direct[k],1);  
  }
}
  sectors -= num_dealloc;
  if (sectors <= 0)
  {
    disk_inode.length = 0;
    return true;    /* code */
  }
  else{
    if(sectors <= INDIRECT_BLOCKS){
      num_dealloc = sectors;
    }
    else{
      num_dealloc = INDIRECT_BLOCKS;
    }
    struct indirect_block_sec indirect_sector;
    if (disk_inode.indirect != 0)
    {
      block_read(fs_device, disk_inode.indirect, &indirect_sector );
      for(int k = 0; k < num_dealloc; k += 1){
        if(indirect_sector.direct[k] != 0){
          free_map_release(indirect_sector.direct[k],1);
        }
      }
      free_map_release(disk_inode.indirect, 1);
      sectors -= num_dealloc;
    } 

  }
  if(sectors<= 0){
    disk_inode.length = 0;
    return true;
  }
  disk_inode.length = sectors*BLOCK_SECTOR_SIZE;
  return false;

}
/* Returns the block device sector that contains byte offset POS
   within INODE.
   Returns -1 if INODE does not contain data for a byte at offset
   POS. */
static block_sector_t
byte_to_sector (const struct inode *inode, off_t pos) 
{
   ASSERT (inode != NULL);
  // if (pos < inode->data.length)
  //   return inode->data.start + pos / BLOCK_SECTOR_SIZE;
  // else
  //   return -1;
  uint32_t block_index = pos/BLOCK_SECTOR_SIZE;
  if(block_index<=122){
    return inode->data.direct[block_index];
  }
  else if(block_index >122 && block_index <= 250){
    block_index -= 123;
    struct indirect_block_sec ibs;
    block_read(fs_device, inode->data.indirect, &ibs);
    return ibs.direct[block_index];
  }
  else{
    struct indirect_block_sec ibs;
    block_index -= 123;
    block_index -= 128;
    block_read(fs_device, inode->data.doubly_indirect, &ibs);
   // pos-=BLOCK_SECTOR_SIZE*(123+128*128);
   // block_index = pos / (BLOCK_SECTOR_SIZE*128);
    block_index /= 128;
    block_read(fs_device, ibs.direct[block_index], &ibs);
    block_index %= 128;
    return ibs.direct[block_index];
  }

  return -1;

}

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/* Initializes the inode module. */
void
inode_init (void) 
{
  list_init (&open_inodes);
}

/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   device.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool
inode_create (block_sector_t sector, off_t length, bool is_directory)
{
  struct inode_disk *disk_inode = NULL;
  bool success = false;

  ASSERT (length >= 0);

  /* If this assertion fails, the inode structure is not exactly
     one sector in size, and you should fix that. */
  ASSERT (sizeof *disk_inode == BLOCK_SECTOR_SIZE);

  disk_inode = calloc (1, sizeof *disk_inode);
  if (disk_inode != NULL)
    {
      size_t sectors = bytes_to_sectors (length);
      disk_inode->length = length;
      disk_inode->magic = INODE_MAGIC;
      disk_inode->is_directory = is_directory;
      if (inode_alloc(disk_inode, length)) 
        {
          block_write (fs_device, sector, disk_inode);
          // if (sectors > 0) 
          //   {
          //     static char zeros[BLOCK_SECTOR_SIZE];
          //     size_t i;
              
          //     for (i = 0; i < sectors; i++) 
          //       block_write (fs_device, disk_inode->start + i, zeros);
          //   }
          success = true; 
        } 
      free (disk_inode);
    }
  //  printf("\n\nSUCCESS %d\n\n", success);
  return success;
}



/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (block_sector_t sector)
{
  struct list_elem *e;
  struct inode *inode;

  /* Check whether this inode is already open. */
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e)) 
    {
      inode = list_entry (e, struct inode, elem);
      if (inode->sector == sector) 
        {
          inode_reopen (inode);
          return inode; 
        }
    }

  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL)
    return NULL;

  /* Initialize. */
  list_push_front (&open_inodes, &inode->elem);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
  block_read (fs_device, inode->sector, &inode->data);
  return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
  if (inode != NULL)
    inode->open_cnt++;
  return inode;
}

/* Returns INODE's inode number. */
block_sector_t
inode_get_inumber (const struct inode *inode)
{
  return inode->sector;
}

/* Closes INODE and writes it to disk.
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode) 
{
  /* Ignore null pointer. */
  if (inode == NULL)
    return;

  /* Release resources if this was the last opener. */
  if (--inode->open_cnt == 0)
    {
      /* Remove from inode list and release lock. */
      list_remove (&inode->elem);
 
      /* Deallocate blocks if removed. */
      if (inode->removed) 
        {
          free_map_release (inode->sector, 1);
          inode_dealloc(inode);
          //free_map_release (inode->data.start,
          //                  bytes_to_sectors (inode->data.length)); 
        }

      free (inode); 
    }
}

/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *inode) 
{
  ASSERT (inode != NULL);
  inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset) 
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;
  uint8_t *bounce = NULL;

  while (size > 0) 
    {
      /* Disk sector to read, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Read full sector directly into caller's buffer. */
          block_read (fs_device, sector_idx, buffer + bytes_read);
        }
      else 
        {
          /* Read sector into bounce buffer, then partially copy
             into caller's buffer. */
          if (bounce == NULL) 
            {
              bounce = malloc (BLOCK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }
          block_read (fs_device, sector_idx, bounce);
          memcpy (buffer + bytes_read, bounce + sector_ofs, chunk_size);
        }
      
      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_read += chunk_size;
    }
  free (bounce);

  return bytes_read;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset) 
{
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;
  uint8_t *bounce = NULL;

  if (inode->deny_write_cnt)
    return 0;

  if(offset+size > inode->data.length){
    //printf("\n\n\nWRITEAT\n\n\n");
    if(!inode_alloc(&inode->data, offset + size)){
      //printf("\n\n\nWRITEAT FAIL\n\n\n");
      return 0;
    }
    
    inode->data.length = offset + size;
    block_write(fs_device, inode->sector, &inode->data);
    //printf("\n\n\nWRITEAT SUCCESSSSS\n\n\n");
  }
  while (size > 0) 
    {
      /* Sector to write, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually write into this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Write full sector directly to disk. */
          block_write (fs_device, sector_idx, buffer + bytes_written);
        }
      else 
        {
          /* We need a bounce buffer. */
          if (bounce == NULL) 
            {
              bounce = malloc (BLOCK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }

          /* If the sector contains data before or after the chunk
             we're writing, then we need to read in the sector
             first.  Otherwise we start with a sector of all zeros. */
          if (sector_ofs > 0 || chunk_size < sector_left) 
            block_read (fs_device, sector_idx, bounce);
          else
            memset (bounce, 0, BLOCK_SECTOR_SIZE);
          memcpy (bounce + sector_ofs, buffer + bytes_written, chunk_size);
          block_write (fs_device, sector_idx, bounce);
        }

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_written += chunk_size;
    }
  free (bounce);

  return bytes_written;
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode) 
{
  inode->deny_write_cnt++;
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode) 
{
  ASSERT (inode->deny_write_cnt > 0);
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  inode->deny_write_cnt--;
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
  return inode->data.length;
}
