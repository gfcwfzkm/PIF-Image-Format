/*
 * flashcache.h
 *
 * Created: 15.02.2021 13:03:56
 */ 

#ifndef FLASHCACHE_H_
#define FLASHCACHE_H_

#include <inttypes.h>
#include <string.h>

#include "../icdriver/spiflash/spiflash.h"

// Enable this if you want to use FATFS *only* with flashcache
#define IMPLEMENT_FATFS_DRIVER

#define CACHE_BLOCKSIZE	512

typedef struct {
	flash_t *_flash;
	uint8_t _buf[FLASH_C_SECTOR_SIZE];
	uint32_t _addr;
	uint8_t _id;
	void *previousCache;
}flashcache_t;

/* Basic Flash Cache I/O tools */
void fc_init(flashcache_t *cache, flash_t *flash_ptr);
flashcache_t *fc_getFlash(uint8_t id);
void fc_sync(flashcache_t *cache);
void fc_read(flashcache_t *cache, uint32_t addr, uint8_t *dataBuf, uint16_t length);
void fc_write(flashcache_t *cache, uint32_t addr, uint8_t const *dataBuf, uint16_t length);

/* Glue functions for FAT FS from here: readblocks, writeblocks, syncblocks*/

void cache_syncBlocks(uint8_t id);
void cache_writeBlocks(uint8_t id, uint32_t block, const uint8_t *src, size_t nb);
void cache_readBlocks(uint8_t id, uint32_t block, uint8_t *dst, size_t nb);


#endif /* FLASHCACHE_H_ */