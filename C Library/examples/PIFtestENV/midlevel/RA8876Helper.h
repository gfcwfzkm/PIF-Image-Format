/*
 * RA8876Helper.h
 *
 * Created: 05.03.2021 09:44:09
 */ 


#ifndef RA8876HELPER_H_
#define RA8876HELPER_H_

#define GESIBUG

#include <inttypes.h>
#include "..\icdriver\RA8876\RA8876.h"
#include "ff.h"
#include "gshell\gshell.h"

#define RAHELPER_USECANVASLAYER	0xFFFFFFFF

#define PIF_COMPRESSION		0x7DDE

void RAHelper_prepareImgUpload(uint32_t layerAddr, uint16_t x0, uint16_t y0, uint16_t width, uint16_t height, enum RA8876_BTE_S0_color bits_per_pixel);

// Monochrome image upload
void RAHelper_sendMonochromeRLE(uint16_t img_length, const __memx uint8_t *img, uint8_t fgCol8, uint8_t bgCol8);

void RAHelper_send8bppRLE(uint16_t img_length, const __memx uint8_t *img);

void RAHelper_copyImage(uint32_t srcAddr, uint16_t src_x, uint16_t src_y, uint32_t destAddr, uint16_t dest_x, uint16_t dest_y, uint16_t imgWidth, uint16_t imgHeight);

void RAHelper_loadBitmapFromFIL(uint32_t layerAddr, uint16_t x0, uint16_t y0, FIL *p_ffd);

void RAHelper_loadBitmapFromPath(uint32_t layerAddr, uint16_t x0, uint16_t y0, const char *str_imagePath);

void RAHelper_displayBitmapFromFlash(uint32_t dispAddr,uint16_t x0, uint16_t y0, uint32_t flashAddr, uint16_t width, uint16_t height);

#endif /* RA8876HELPER_H_ */