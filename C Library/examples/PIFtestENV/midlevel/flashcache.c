/*
 * flashcache.c
 *
 * Created: 15.02.2021 13:03:42
 */ 

#include "flashcache.h"

#include "ff.h"
#include "diskio.h"

#define INV_ADDR	0xFFFFFFFF
#define fc_min(a,b)	((a)<(b)?(a):(b))

flashcache_t *_newestFlash = NULL;

static inline uint32_t sector_of(uint32_t addr) {
	return addr & ~(FLASH_C_SECTOR_SIZE - 1);
}

static inline uint32_t offset_of(uint32_t addr) {
	return addr & (FLASH_C_SECTOR_SIZE - 1);
}

void fc_init(flashcache_t *cache, flash_t *flash_ptr)
{
	cache->_flash = flash_ptr;
	if (_newestFlash != NULL)
	{
		cache->_id = _newestFlash->_id + 1;
		cache->previousCache = _newestFlash; 
	}
	else
	{
		cache->_id = 0;
	}
	_newestFlash = cache;
	
	cache->_addr = INV_ADDR;
}

// Can be used to get the right flash cache struct depending on the drive number from FATFS,
// thus allowing for multiple flashcaches to be used with FATFS
flashcache_t *fc_getFlash(uint8_t id)
{
	flashcache_t *searchCache = _newestFlash;
	
	while(searchCache->_id)
	{
		if (searchCache->_id == id)
		{
			// struct found!
			break;
		}
		else if (searchCache->_id < id)
		{
			// Number higher than the lastest struct -> invalid ID!
			searchCache = NULL;
			break;
		}
		else
		{
			// Keep searching
			searchCache = (flashcache_t*)searchCache->previousCache;
		}
	}
	
	return searchCache;
}

void fc_sync(flashcache_t *cache)
{
	if (cache->_addr != INV_ADDR)
	{
		flash_eraseSector(cache->_flash, cache->_addr / FLASH_C_SECTOR_SIZE);
		flash_writeBuffer(cache->_flash, cache->_addr, cache->_buf, FLASH_C_SECTOR_SIZE);
		cache->_addr = INV_ADDR;
	}
}

void fc_read(flashcache_t *cache, uint32_t addr, uint8_t *dataBuf, uint16_t length)
{
	int32_t dst_off;
	int32_t scr_off = 0;
	int32_t cache_bytes;
	uint32_t copied;
	
	if ((cache->_addr != INV_ADDR) &&
		!(addr < cache->_addr && addr + length <= cache->_addr) &&
		!(addr >= cache->_addr + FLASH_C_SECTOR_SIZE))
	{
		dst_off = cache->_addr - addr;
		
		if (dst_off < 0)
		{
			scr_off = -dst_off;
			dst_off = 0;
		}
		
		cache_bytes = fc_min((int32_t)(FLASH_C_SECTOR_SIZE - scr_off), (int32_t)(length - dst_off));
		
		if (dst_off)
		{
			flash_readBuffer(cache->_flash, addr, dataBuf, dst_off);
		}
		
		memcpy(dataBuf + dst_off, cache->_buf + scr_off, cache_bytes);
		
		copied = (uint32_t)(dst_off + cache_bytes);
		if (copied < length)
		{
			flash_readBuffer(cache->_flash, addr + copied, dataBuf + copied, length - copied);
		}
	}
	else
	{
		flash_readBuffer(cache->_flash, addr, dataBuf, length);
	}
}

void fc_write(flashcache_t *cache, uint32_t addr, uint8_t const *dataBuf, uint16_t length)
{
	uint32_t remain = length;
	uint32_t sector_addr;
	uint32_t offset;
	uint32_t wr_bytes;
	
	while(remain){
		sector_addr = sector_of(addr);
		offset = offset_of(addr);
		
		wr_bytes = FLASH_C_SECTOR_SIZE - offset;
		wr_bytes = fc_min(remain, wr_bytes);
		
		if (sector_addr != cache->_addr)
		{
			fc_sync(cache);
			cache->_addr = sector_addr;
			flash_readBuffer(cache->_flash, sector_addr, cache->_buf, FLASH_C_SECTOR_SIZE);
		}
		
		memcpy(cache->_buf + offset, dataBuf, wr_bytes);
		
		dataBuf += wr_bytes;
		remain -= wr_bytes;
		addr += wr_bytes;
	}
}

void cache_syncBlocks(uint8_t id)
{
	flashcache_t *cache = fc_getFlash(id);
	fc_sync(cache);
}

void cache_writeBlocks(uint8_t id, uint32_t block, const uint8_t *src, size_t nb)
{
	flashcache_t *cache = fc_getFlash(id);
	fc_write(cache, block * CACHE_BLOCKSIZE, src, CACHE_BLOCKSIZE * nb);
}

void cache_readBlocks(uint8_t id, uint32_t block, uint8_t *dst, size_t nb)
{
	flashcache_t *cache = fc_getFlash(id);
	fc_read(cache, block * CACHE_BLOCKSIZE, dst, CACHE_BLOCKSIZE * nb);
}

#ifdef IMPLEMENT_FATFS_DRIVER
#ifdef _DISKIO_DEFINED
// FATFS diskio

DSTATUS disk_status (BYTE pdrv){
	// not implemented
	return 0;
}

DSTATUS disk_initialize(BYTE pdrv){
	return 0;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count){
	cache_readBlocks(pdrv, sector, buff, count);
	return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count){
	cache_writeBlocks(pdrv, sector, buff, count);
	return RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff){
	switch (cmd)
	{
		case CTRL_SYNC:
			cache_syncBlocks(pdrv);
			return RES_OK;
		case GET_SECTOR_COUNT:
			if (fc_getFlash(pdrv) == NULL)	return RES_PARERR;
			*((DWORD*) buff) = fc_getFlash(pdrv)->_flash->total_size / CACHE_BLOCKSIZE;
			return RES_OK;
		case GET_SECTOR_SIZE:
			*((WORD*) buff) = CACHE_BLOCKSIZE;
			return RES_OK;
		case GET_BLOCK_SIZE:
			*((DWORD*) buff) = FLASH_C_SECTOR_SIZE/CACHE_BLOCKSIZE;
			return RES_OK;
		default:
			return RES_PARERR;
	}
}
#endif
#endif