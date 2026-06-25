#include "drivers/ata.h"
#include "cpu/ports.h"

//primary ATA bus I/O ports
#define ATA_DATA      0x1F0
#define ATA_ERR       0x1F1
#define ATA_SECCOUNT  0x1F2
#define ATA_LBA_LO    0x1F3
#define ATA_LBA_MID   0x1F4
#define ATA_LBA_HI    0x1F5
#define ATA_DRIVE     0x1F6
#define ATA_STATUS    0x1F7
#define ATA_COMMAND   0x1F7

#define ST_BSY  0x80
#define ST_DRDY 0x40
#define ST_DRQ  0x08
#define ST_ERR  0x01

#define CMD_READ  0x20
#define CMD_WRITE 0x30
#define CMD_FLUSH 0xE7

//the data disk is the slave on the primary bus -> drive/head base 0xF0
#define DRIVE_SLAVE 0x10

static int wait_ready(void) {
    //spin while busy, then until DRQ or error (bounded so a missing disk can't
    //hang the kernel forever)
    int spin = 1000000;
    while ((inb(ATA_STATUS) & ST_BSY) && spin-- > 0) ;
    spin = 1000000;
    for (;;) {
        u8 s = inb(ATA_STATUS);
        if (s & ST_ERR) return -1;
        if (s & ST_DRQ) return 0;
        if (spin-- <= 0) return -1;
    }
}

static void select_and_seek(u32 lba, u8 count) {
    while (inb(ATA_STATUS) & ST_BSY) ;
    outb(ATA_DRIVE, 0xE0 | DRIVE_SLAVE | ((lba >> 24) & 0x0F)); //LBA mode, slave
    outb(ATA_SECCOUNT, count);
    outb(ATA_LBA_LO,  (u8)(lba & 0xFF));
    outb(ATA_LBA_MID, (u8)((lba >> 8) & 0xFF));
    outb(ATA_LBA_HI,  (u8)((lba >> 16) & 0xFF));
}

void ata_init(void) {
    //select the drive once so STATUS reads are meaningful
    outb(ATA_DRIVE, 0xE0 | DRIVE_SLAVE);
    for (volatile int i = 0; i < 1000; i++) ;
}

int ata_read_sector(u32 lba, void *buf) {
    select_and_seek(lba, 1);
    outb(ATA_COMMAND, CMD_READ);
    if (wait_ready() != 0) return -1;
    insw(ATA_DATA, buf, ATA_SECTOR_SIZE / 2);
    return 0;
}

int ata_write_sector(u32 lba, const void *buf) {
    select_and_seek(lba, 1);
    outb(ATA_COMMAND, CMD_WRITE);
    if (wait_ready() != 0) return -1;
    outsw(ATA_DATA, buf, ATA_SECTOR_SIZE / 2);
    //flush the write cache so data actually reaches the disk image
    outb(ATA_COMMAND, CMD_FLUSH);
    while (inb(ATA_STATUS) & ST_BSY) ;
    return 0;
}
