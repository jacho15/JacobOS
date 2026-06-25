#ifndef DRIVERS_ATA_H
#define DRIVERS_ATA_H

#include "cpu/types.h"

#define ATA_SECTOR_SIZE 512

//synchronous (polling) PIO driver for the primary-bus IDE slave, which is the
//persistent data disk the Makefile attaches at if=ide,index=1. one sector per
//call keeps it simple; the block layer batches as needed.
void ata_init(void);
//read one 512-byte sector at LBA into buf (must hold 512 bytes)
int  ata_read_sector(u32 lba, void *buf);
//write one 512-byte sector at LBA from buf
int  ata_write_sector(u32 lba, const void *buf);

#endif
