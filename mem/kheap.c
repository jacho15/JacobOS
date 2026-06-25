#include "mem/kheap.h"

//first-fit free-list allocator. each block is a header followed by its payload;
//blocks are kept in a single address-ordered linked list so freeing can coalesce
//neighbours.
typedef struct header {
    size_t        size;   //payload bytes
    int           free;
    struct header *next;
} header_t;

#define HDR sizeof(header_t)
#define ALIGN8(x) (((x) + 7u) & ~7u)

static header_t *heap_head;

void kheap_init(void) {
    heap_head = (header_t*)HEAP_START;
    heap_head->size = HEAP_SIZE - HDR;
    heap_head->free = 1;
    heap_head->next = 0;
}

void *kmalloc(size_t size) {
    if (size == 0) return 0;
    size = ALIGN8(size);

    for (header_t *cur = heap_head; cur; cur = cur->next) {
        if (!cur->free || cur->size < size) continue;

        //split off the remainder if there is room for another block
        if (cur->size >= size + HDR + 8) {
            header_t *nb = (header_t*)((u8*)cur + HDR + size);
            nb->size = cur->size - size - HDR;
            nb->free = 1;
            nb->next = cur->next;
            cur->next = nb;
            cur->size = size;
        }
        cur->free = 0;
        return (u8*)cur + HDR;
    }
    return 0; //out of heap
}

void kfree(void *ptr) {
    if (!ptr) return;
    header_t *h = (header_t*)((u8*)ptr - HDR);
    h->free = 1;

    //merge runs of adjacent free blocks
    for (header_t *cur = heap_head; cur; cur = cur->next) {
        while (cur->free && cur->next && cur->next->free) {
            cur->size += HDR + cur->next->size;
            cur->next = cur->next->next;
        }
    }
}

u32 kheap_used(void) {
    u32 used = 0;
    for (header_t *cur = heap_head; cur; cur = cur->next)
        if (!cur->free) used += cur->size;
    return used;
}

u32 kheap_free(void) {
    u32 freeb = 0;
    for (header_t *cur = heap_head; cur; cur = cur->next)
        if (cur->free) freeb += cur->size;
    return freeb;
}
