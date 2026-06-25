#ifndef FS_SFS_H
#define FS_SFS_H

#include "cpu/types.h"

//a tiny persistent, hierarchical filesystem on the ATA data disk.
//inodes are referenced by number; the root directory is inode 0. every
//directory holds "." and ".." entries so path traversal (cd ..) just works.

#define SFS_FREE 0
#define SFS_FILE 1
#define SFS_DIR  2

#define SFS_ROOT 0
#define SFS_NAME_MAX 27   //chars; dirent stores name[28]

void sfs_init(void);                                   //mount, or format if blank

int  sfs_lookup(int dir, const char *name);            //-> inode# or -1
int  sfs_create(int dir, const char *name, int type);  //-> inode# or -1
int  sfs_readdir(int dir, int index, char *name_out);  //-> child inode# or -1
int  sfs_unlink(int dir, const char *name);            //-> 0 ok, -1 fail

int  sfs_read(int inode, char *buf, u32 max);          //-> bytes read
int  sfs_write(int inode, const char *buf, u32 len);   //overwrite; -> bytes

int  sfs_type(int inode);
u32  sfs_size(int inode);

#endif
