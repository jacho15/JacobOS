#include "mem/pmm.h"

#define NUM_FRAMES (PHYS_MEM_SIZE / FRAME_SIZE)   //8192 frames over 32MB

//one bit per frame: 1 = used, 0 = free
static u32 bitmap[NUM_FRAMES / 32];
static u32 used_frames;

static void set_frame(u32 frame)   { bitmap[frame / 32] |=  (1u << (frame % 32)); }
static void clear_frame(u32 frame) { bitmap[frame / 32] &= ~(1u << (frame % 32)); }
static int  test_frame(u32 frame)  { return bitmap[frame / 32] & (1u << (frame % 32)); }

void pmm_init(void) {
    for (u32 i = 0; i < NUM_FRAMES / 32; i++) bitmap[i] = 0;
    used_frames = 0;
    //reserve everything below PMM_RESERVED_END
    u32 reserved = PMM_RESERVED_END / FRAME_SIZE;
    for (u32 i = 0; i < reserved; i++) { set_frame(i); used_frames++; }
}

u32 pmm_alloc_frame(void) {
    for (u32 i = 0; i < NUM_FRAMES; i++) {
        if (!test_frame(i)) {
            set_frame(i);
            used_frames++;
            return i * FRAME_SIZE;
        }
    }
    return 0; //out of physical memory
}

void pmm_free_frame(u32 addr) {
    u32 frame = addr / FRAME_SIZE;
    if (frame >= NUM_FRAMES) return;
    if (test_frame(frame)) {
        clear_frame(frame);
        used_frames--;
    }
}

u32 pmm_total_frames(void) { return NUM_FRAMES; }
u32 pmm_used_frames(void)  { return used_frames; }
