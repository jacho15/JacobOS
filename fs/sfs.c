#include "fs/sfs.h"
#include "fs/bcache.h"
#include "lib/string.h"

#define BLOCK_SIZE         512
#define SFS_MAGIC          0x4A534653u   //'JSFS'
#define DIRECT             14
#define MAX_INODES         64
#define INODES_PER_BLOCK   (BLOCK_SIZE / 64)        //8
#define INODE_TABLE_START  1
#define INODE_TABLE_BLOCKS (MAX_INODES / INODES_PER_BLOCK)  //8
#define BITMAP_BLOCK       9
#define DATA_START         16
#define MAX_DATA_BLOCKS    2048
#define DIRENTS_PER_BLOCK  (BLOCK_SIZE / 32)        //16

typedef struct {
    u32 type;
    u32 size;
    u32 blocks[DIRECT];
} inode_t;   //64 bytes

typedef struct {
    u32  inum1;            //child inode + 1; 0 = free slot
    char name[28];
} dirent_t;  //32 bytes

typedef struct {
    u32 magic;
    u32 num_inodes;
    u32 data_start;
    u32 max_data_blocks;
} superblock_t;

//--- block layer: now goes through the write-back block cache --------------
static void read_block(u32 blk, void *buf)        { bcache_read(blk, buf); }
static void write_block(u32 blk, const void *buf) { bcache_write(blk, buf); }

//--- inode access ----------------------------------------------------------
static void read_inode(int i, inode_t *out) {
    u8 buf[BLOCK_SIZE];
    u32 blk = INODE_TABLE_START + i / INODES_PER_BLOCK;
    read_block(blk, buf);
    memcpy(out, buf + (i % INODES_PER_BLOCK) * 64, sizeof(inode_t));
}

static void write_inode(int i, const inode_t *in) {
    u8 buf[BLOCK_SIZE];
    u32 blk = INODE_TABLE_START + i / INODES_PER_BLOCK;
    read_block(blk, buf);
    memcpy(buf + (i % INODES_PER_BLOCK) * 64, in, sizeof(inode_t));
    write_block(blk, buf);
}

static int alloc_inode(void) {
    inode_t n;
    for (int i = 0; i < MAX_INODES; i++) {
        read_inode(i, &n);
        if (n.type == SFS_FREE) return i;
    }
    return -1;
}

//--- data block bitmap -----------------------------------------------------
static u32 alloc_block(void) {
    u8 bm[BLOCK_SIZE];
    read_block(BITMAP_BLOCK, bm);
    for (u32 i = 0; i < MAX_DATA_BLOCKS; i++) {
        if (!(bm[i / 8] & (1 << (i % 8)))) {
            bm[i / 8] |= (1 << (i % 8));
            write_block(BITMAP_BLOCK, bm);
            //zero the freshly allocated data block
            u8 z[BLOCK_SIZE];
            memset(z, 0, BLOCK_SIZE);
            write_block(DATA_START + i, z);
            return DATA_START + i;
        }
    }
    return 0;
}

static void free_block(u32 absblk) {
    if (absblk < DATA_START) return;
    u32 i = absblk - DATA_START;
    u8 bm[BLOCK_SIZE];
    read_block(BITMAP_BLOCK, bm);
    bm[i / 8] &= ~(1 << (i % 8));
    write_block(BITMAP_BLOCK, bm);
}

//--- directory helpers -----------------------------------------------------
static int dir_lookup(int dir, const char *name) {
    inode_t d;
    read_inode(dir, &d);
    if (d.type != SFS_DIR) return -1;
    u8 buf[BLOCK_SIZE];
    for (u32 b = 0; b < DIRECT && d.blocks[b]; b++) {
        read_block(d.blocks[b], buf);
        dirent_t *de = (dirent_t*)buf;
        for (u32 e = 0; e < DIRENTS_PER_BLOCK; e++) {
            if (de[e].inum1 && strcmp(de[e].name, name) == 0)
                return (int)de[e].inum1 - 1;
        }
    }
    return -1;
}

static int dir_add(int dir, const char *name, int child) {
    inode_t d;
    read_inode(dir, &d);
    u8 buf[BLOCK_SIZE];

    //reuse a free slot in an existing block
    for (u32 b = 0; b < DIRECT && d.blocks[b]; b++) {
        read_block(d.blocks[b], buf);
        dirent_t *de = (dirent_t*)buf;
        for (u32 e = 0; e < DIRENTS_PER_BLOCK; e++) {
            if (de[e].inum1 == 0) {
                de[e].inum1 = (u32)child + 1;
                strncpy(de[e].name, name, 27);
                de[e].name[27] = 0;
                write_block(d.blocks[b], buf);
                return 0;
            }
        }
    }
    //otherwise grow the directory by one block
    for (u32 b = 0; b < DIRECT; b++) {
        if (!d.blocks[b]) {
            u32 nb = alloc_block();
            if (!nb) return -1;
            d.blocks[b] = nb;
            d.size += BLOCK_SIZE;
            write_inode(dir, &d);
            memset(buf, 0, BLOCK_SIZE);
            dirent_t *de = (dirent_t*)buf;
            de[0].inum1 = (u32)child + 1;
            strncpy(de[0].name, name, 27);
            de[0].name[27] = 0;
            write_block(nb, buf);
            return 0;
        }
    }
    return -1;
}

static int dir_remove(int dir, const char *name) {
    inode_t d;
    read_inode(dir, &d);
    u8 buf[BLOCK_SIZE];
    for (u32 b = 0; b < DIRECT && d.blocks[b]; b++) {
        read_block(d.blocks[b], buf);
        dirent_t *de = (dirent_t*)buf;
        for (u32 e = 0; e < DIRENTS_PER_BLOCK; e++) {
            if (de[e].inum1 && strcmp(de[e].name, name) == 0) {
                de[e].inum1 = 0;
                write_block(d.blocks[b], buf);
                return 0;
            }
        }
    }
    return -1;
}

//--- format / mount --------------------------------------------------------
static void format(void) {
    u8 buf[BLOCK_SIZE];

    //superblock
    memset(buf, 0, BLOCK_SIZE);
    superblock_t *sb = (superblock_t*)buf;
    sb->magic = SFS_MAGIC;
    sb->num_inodes = MAX_INODES;
    sb->data_start = DATA_START;
    sb->max_data_blocks = MAX_DATA_BLOCKS;
    write_block(0, buf);

    //clear inode table and bitmap
    memset(buf, 0, BLOCK_SIZE);
    for (u32 b = 0; b < INODE_TABLE_BLOCKS; b++)
        write_block(INODE_TABLE_START + b, buf);
    write_block(BITMAP_BLOCK, buf);

    //root directory = inode 0
    inode_t root;
    memset(&root, 0, sizeof(root));
    root.type = SFS_DIR;
    write_inode(SFS_ROOT, &root);
    dir_add(SFS_ROOT, ".",  SFS_ROOT);
    dir_add(SFS_ROOT, "..", SFS_ROOT);
}

void sfs_init(void) {
    u8 buf[BLOCK_SIZE];
    read_block(0, buf);
    superblock_t *sb = (superblock_t*)buf;
    if (sb->magic != SFS_MAGIC) format();
}

//--- public API ------------------------------------------------------------
int sfs_lookup(int dir, const char *name) { return dir_lookup(dir, name); }

int sfs_type(int inode) { inode_t n; read_inode(inode, &n); return (int)n.type; }
u32 sfs_size(int inode) { inode_t n; read_inode(inode, &n); return n.size; }

int sfs_create(int dir, const char *name, int type) {
    if (dir_lookup(dir, name) >= 0) return -1; //already exists
    int ino = alloc_inode();
    if (ino < 0) return -1;

    inode_t n;
    memset(&n, 0, sizeof(n));
    n.type = (u32)type;
    write_inode(ino, &n);

    if (type == SFS_DIR) {
        dir_add(ino, ".",  ino);
        dir_add(ino, "..", dir);
    }
    if (dir_add(dir, name, ino) != 0) return -1;
    return ino;
}

int sfs_readdir(int dir, int index, char *name_out) {
    inode_t d;
    read_inode(dir, &d);
    if (d.type != SFS_DIR) return -1;
    u8 buf[BLOCK_SIZE];
    int seen = 0;
    for (u32 b = 0; b < DIRECT && d.blocks[b]; b++) {
        read_block(d.blocks[b], buf);
        dirent_t *de = (dirent_t*)buf;
        for (u32 e = 0; e < DIRENTS_PER_BLOCK; e++) {
            if (de[e].inum1) {
                if (seen == index) {
                    strcpy(name_out, de[e].name);
                    return (int)de[e].inum1 - 1;
                }
                seen++;
            }
        }
    }
    return -1;
}

int sfs_write(int inode, const char *data, u32 len) {
    inode_t n;
    read_inode(inode, &n);
    if (n.type != SFS_FILE) return -1;

    //free any existing blocks, then write fresh
    for (u32 b = 0; b < DIRECT; b++) {
        if (n.blocks[b]) { free_block(n.blocks[b]); n.blocks[b] = 0; }
    }
    u32 max_bytes = DIRECT * BLOCK_SIZE;
    if (len > max_bytes) len = max_bytes;

    u32 written = 0;
    u32 b = 0;
    u8 buf[BLOCK_SIZE];
    while (written < len) {
        u32 chunk = len - written;
        if (chunk > BLOCK_SIZE) chunk = BLOCK_SIZE;
        u32 nb = alloc_block();
        if (!nb) break;
        n.blocks[b] = nb;
        memset(buf, 0, BLOCK_SIZE);
        memcpy(buf, data + written, chunk);
        write_block(nb, buf);
        written += chunk;
        b++;
    }
    n.size = written;
    write_inode(inode, &n);
    return (int)written;
}

int sfs_read(int inode, char *out, u32 max) {
    inode_t n;
    read_inode(inode, &n);
    u32 len = n.size;
    if (len > max) len = max;

    u32 done = 0;
    u32 b = 0;
    u8 buf[BLOCK_SIZE];
    while (done < len && b < DIRECT && n.blocks[b]) {
        u32 chunk = len - done;
        if (chunk > BLOCK_SIZE) chunk = BLOCK_SIZE;
        read_block(n.blocks[b], buf);
        memcpy(out + done, buf, chunk);
        done += chunk;
        b++;
    }
    return (int)done;
}

int sfs_unlink(int dir, const char *name) {
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) return -1;
    int ino = dir_lookup(dir, name);
    if (ino < 0) return -1;

    inode_t n;
    read_inode(ino, &n);
    if (n.type == SFS_DIR) {
        //refuse to remove a non-empty directory (more than "." and "..")
        char tmp[28];
        int count = 0;
        for (int i = 0; sfs_readdir(ino, i, tmp) >= 0; i++) count++;
        if (count > 2) return -1;
    }
    for (u32 b = 0; b < DIRECT; b++)
        if (n.blocks[b]) free_block(n.blocks[b]);
    memset(&n, 0, sizeof(n));
    write_inode(ino, &n);
    return dir_remove(dir, name);
}
