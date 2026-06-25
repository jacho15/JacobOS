#ifndef FS_BCACHE_H
#define FS_BCACHE_H

#include "cpu/types.h"

//a small write-back cache of disk blocks sitting between the filesystem and the
//ATA driver. read hits avoid a disk read; writes stay in RAM (marked dirty)
//until bcache_sync() flushes them, mirroring a real OS page cache.
void bcache_init(void);
void bcache_read(u32 blk, void *buf);
void bcache_write(u32 blk, const void *buf);
void bcache_sync(void);   //flush all dirty blocks to disk

u32  bcache_hits(void);
u32  bcache_misses(void);

#endif
