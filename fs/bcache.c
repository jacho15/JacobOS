#include "fs/bcache.h"
#include "drivers/ata.h"
#include "lib/string.h"

#define SLOTS      32
#define BLOCK_SIZE 512

typedef struct {
    u32 blk;
    int valid;
    int dirty;
    u8  data[BLOCK_SIZE];
} slot_t;

static slot_t slots[SLOTS];
static u32 hand;   //clock hand for round-robin eviction
static u32 hits, misses;

void bcache_init(void) {
    for (int i = 0; i < SLOTS; i++) { slots[i].valid = 0; slots[i].dirty = 0; }
    hand = hits = misses = 0;
}

static slot_t *find(u32 blk) {
    for (int i = 0; i < SLOTS; i++)
        if (slots[i].valid && slots[i].blk == blk) return &slots[i];
    return 0;
}

//pick a slot to (re)use, flushing it first if it holds dirty data
static slot_t *evict(void) {
    slot_t *s = &slots[hand];
    hand = (hand + 1) % SLOTS;
    if (s->valid && s->dirty) ata_write_sector(s->blk, s->data);
    return s;
}

void bcache_read(u32 blk, void *buf) {
    slot_t *s = find(blk);
    if (s) {
        hits++;
    } else {
        misses++;
        s = evict();
        ata_read_sector(blk, s->data);
        s->blk = blk;
        s->valid = 1;
        s->dirty = 0;
    }
    memcpy(buf, s->data, BLOCK_SIZE);
}

void bcache_write(u32 blk, const void *buf) {
    slot_t *s = find(blk);
    if (!s) { s = evict(); s->blk = blk; s->valid = 1; }
    memcpy(s->data, buf, BLOCK_SIZE);
    s->dirty = 1;   //write-back: stays in RAM until sync
}

void bcache_sync(void) {
    for (int i = 0; i < SLOTS; i++) {
        if (slots[i].valid && slots[i].dirty) {
            ata_write_sector(slots[i].blk, slots[i].data);
            slots[i].dirty = 0;
        }
    }
}

u32 bcache_hits(void)   { return hits; }
u32 bcache_misses(void) { return misses; }
