/*
 * RA8876Helper.c
 *
 * Created: 05.03.2021 09:44:17
 */
#include "RA8876Helper.h"

void RAHelper_prepareImgUpload(uint32_t layerAddr, uint16_t x0, uint16_t y0, uint16_t width, uint16_t height, enum RA8876_BTE_S0_color bits_per_pixel)
{
	while(RA8876_readStatus() & RA8876_CORE_BUSY);
	RA8876_setMode(GRAPHMODE);
	
	RA8876_BTE_Colors(bits_per_pixel, BTE_S1_Color_16bpp, BTE_Dest_Color_16bpp);
	RA8876_BTE_S0_Address(layerAddr);
	RA8876_BTE_S0_Coords(x0,y0);
	RA8876_BTE_S0_Width(RA8876_WIDTH);
	RA8876_BTE_Dest_Address(layerAddr);
	RA8876_BTE_Dest_Coords(x0,y0);
	RA8876_BTE_Dest_Width(RA8876_WIDTH);
	RA8876_BTE_WindowSize(width, height);
	RA8876_BTE_ROP_Code(BTE_MPU_WRITE_w_ROP, ROP_S0);
	RA8876_BTE_enable();
	
	RA8876_writeCMD(RA8876_MRWDP);
}

// Monochrome image upload
void RAHelper_sendMonochromeRLE(uint16_t img_length, const __memx uint8_t *img, uint8_t fgCol8, uint8_t bgCol8)
{
	uint16_t imgdata_cnt;
	uint8_t img_data;
	int8_t rle_inst = 0;
	int8_t bit_cnt = 0;
	
	for (imgdata_cnt = 0; imgdata_cnt < img_length; imgdata_cnt++)
	{
		// load next rle-compressed image data from program memory
		img_data = img[imgdata_cnt];
		
		if (rle_inst > 0)
		{
			// If rle_inst is a positive number, repeat the image data rle_inst -times.
			for (; rle_inst > 0; rle_inst--)
			{
				for (bit_cnt = 7; bit_cnt >= 0; bit_cnt--)
				{
					if (img_data & (1<<bit_cnt))
					{
						RA8876_writeData(bgCol8);
					}
					else
					{
						RA8876_writeData(fgCol8);
					}
				}
			}
		}
		else if (rle_inst < 0)
		{
			// if rle_inst is negative, the next -1*(rle_inst) amount of bytes are uncompressed, so just send those.
			for (bit_cnt = 7; bit_cnt >= 0; bit_cnt--)
			{
				if (img_data & (1<<bit_cnt))
				{
					RA8876_writeData(bgCol8);
				}
				else
				{
					RA8876_writeData(fgCol8);
				}
			}
			rle_inst++;
		}
		else
		{
			// if rle_inst is 0, it's time to load the next instruction
			rle_inst = (int8_t)img_data;
		}
	}
}

void RAHelper_send8bppRLE(uint16_t img_length, const __memx uint8_t *img)
{
	uint16_t imgdata_cnt;
	uint8_t img_data;
	int8_t rle_inst = 0;
	
	for (imgdata_cnt = 0; imgdata_cnt < img_length; imgdata_cnt++)
	{
		// load next rle-compressed image data from program memory
		img_data = img[imgdata_cnt];
		
		if (rle_inst > 0)
		{
			// If rle_inst is a positive number, repeat the image data rle_inst -times.
			for (; rle_inst > 0; rle_inst--)
			{
				RA8876_writeData(img_data);
			}
		}
		else if (rle_inst < 0)
		{
			// if rle_inst is negative, the next -1*(rle_inst) amount of bytes are uncompressed, so just send those.
			RA8876_writeData(img_data);
			rle_inst++;
		}
		else
		{
			// if rle_inst is 0, it's time to load the next instruction
			rle_inst = (int8_t)img_data;
		}
	}
}

void RAHelper_copyImage(uint32_t srcAddr, uint16_t src_x, uint16_t src_y, uint32_t destAddr, uint16_t dest_x, uint16_t dest_y, uint16_t imgWidth, uint16_t imgHeight)
{
	while(RA8876_readStatus() & RA8876_CORE_BUSY);
	
	RA8876_BTE_S0_Address(srcAddr);
	RA8876_BTE_S0_Coords(src_x,src_y);
	RA8876_BTE_S0_Width(RA8876_WIDTH);
	
	RA8876_BTE_Dest_Address(destAddr);
	RA8876_BTE_Dest_Coords(dest_x,dest_y);
	RA8876_BTE_Dest_Width(RA8876_WIDTH);
	RA8876_BTE_WindowSize(imgWidth,imgHeight);
	
	RA8876_BTE_Colors(BTE_S0_Color_16bpp, BTE_S1_Color_16bpp, BTE_Dest_Color_16bpp);
	RA8876_BTE_ROP_Code(BTE_MEM_COPY_MOVE_w_ROP, ROP_S0);
	RA8876_BTE_enable();
	
	while(RA8876_readStatus() & RA8876_CORE_BUSY);
}

uint8_t _ff_read8(FIL *p_ffd)
{
	uint8_t readByte;
	UINT recBytes;
	
	f_read(p_ffd, &readByte, 1, &recBytes);
	
	return readByte;
}

uint16_t _ff_read16(FIL *p_ffd)
{
	uint8_t readByte[2];
	UINT recBytes;
	
	f_read(p_ffd, &readByte, 2, &recBytes);
	
	return (uint16_t)readByte[1] << 8 | readByte[0];
}

uint32_t _ff_read24(FIL *p_ffd)
{
	uint8_t readByte[3];
	UINT recBytes;
	
	f_read(p_ffd, &readByte, 3, &recBytes);
	
	return (uint32_t)readByte[2] << 16 | (uint32_t)readByte[1] << 8 | readByte[0];
}

uint32_t _ff_read32(FIL *p_ffd)
{
	uint8_t readByte[4];
	UINT recBytes;
	
	f_read(p_ffd, &readByte, 4, &recBytes);
	
	return (uint32_t)readByte[3] << 24 | (uint32_t)readByte[2] << 16 | (uint32_t)readByte[1] << 8 | readByte[0];
}

void RAHelper_loadBitmapFromFIL(uint32_t layerAddr, uint16_t x0, uint16_t y0, FIL *p_ffd)
{
	uint32_t u32_bmpOffset, u32_fileSize, u32_bmpSize;
	int32_t s32_bmpHeight;
	int32_t s32_bmpWidth;
	uint8_t image_bpp;
	uint8_t compression;
	uint32_t u32_filePos;
	uint32_t u32_pixel;
	uint8_t b_flip = 1;
	uint32_t w,h,col,row,rowSize;
		
	f_lseek(p_ffd, 0);
	if (_ff_read16(p_ffd) == 0x4D42)
	{
		u32_fileSize = _ff_read32(p_ffd);
		_ff_read32(p_ffd);	// dummy read
		u32_bmpOffset = _ff_read32(p_ffd);
		_ff_read32(p_ffd);	// dummy read
		s32_bmpWidth = _ff_read32(p_ffd);
		s32_bmpHeight = _ff_read32(p_ffd);
		_ff_read16(p_ffd);	// dummy read
		image_bpp = (uint8_t)_ff_read16(p_ffd);
		compression = (uint8_t)_ff_read32(p_ffd);
		u32_bmpSize = _ff_read32(p_ffd);
		
#ifdef GESIBUG
		gs_log_f(GLOG_INFO, "Lade Bild...");
		gs_log_f(GLOG_INFO, "Dateigrösse: %"PRIu32" B, Bildgrösse: %"PRIu32 " B", u32_fileSize, u32_bmpSize);
		gs_log_f(GLOG_INFO, "Breite: %"PRIu32" Höhe: %"PRId32, s32_bmpWidth, s32_bmpHeight);
		gs_log_f(GLOG_INFO, "Offset: %"PRIu32" BPP: %"PRIu8" Kompression: %"PRIu8,
			u32_bmpOffset, image_bpp, compression);
#endif

		if (((image_bpp == 16) || (image_bpp == 24)) && ((compression == 0) || (compression == 3)))
		{
			if (s32_bmpHeight < 0)
			{
				b_flip = 0;
				s32_bmpHeight = -s32_bmpHeight;
			}
			w = s32_bmpWidth;
			h = s32_bmpHeight;
			
			if ((x0 + w - 1) >= RA8876_WIDTH)	w = RA8876_WIDTH - x0;
			if ((y0 + h - 1) >= RA8876_HEIGHT)	h = RA8876_HEIGHT - y0;
			
			rowSize = (s32_bmpWidth * (image_bpp / 8) % 4) + (s32_bmpWidth * (image_bpp / 8));
#ifdef GESIBUG
			gs_log_f(GLOG_OK, "Bild OK! Zeilengrösse: %"PRIu32" B", rowSize);
#endif
			if (layerAddr == RAHELPER_USECANVASLAYER)
			{
				layerAddr = RA8876_readReg(RA8876_CVSSA0);
				layerAddr |= (uint32_t)RA8876_readReg(RA8876_CVSSA1) << 8;
				layerAddr |= (uint32_t)RA8876_readReg(RA8876_CVSSA2) << 16;
				layerAddr |= (uint32_t)RA8876_readReg(RA8876_CVSSA3) << 24;
			}
			RAHelper_prepareImgUpload(layerAddr, x0, y0, w, h,
					(image_bpp == 16) ? BTE_S0_Color_16bpp : BTE_S0_Color_24bpp);
				
			for (row = 0; row < h; row ++)
			{
				if (b_flip)
				{
					u32_filePos = ((s32_bmpHeight - 1 - row) * rowSize) + u32_bmpOffset;
				}
				else
				{
					u32_filePos = (row * rowSize) + u32_bmpOffset;
				}
				
				if (f_tell(p_ffd) != u32_filePos)
				{
						f_lseek(p_ffd, u32_filePos);
				}
				
				for (col = 0; col < w; col++)
				{
					if (image_bpp == 16)
					{
						u32_pixel = _ff_read16(p_ffd);
						RA8876_writeData(u32_pixel);
						RA8876_writeData(u32_pixel >> 8);
						
					}
					else if (image_bpp == 24)
					{
						u32_pixel = _ff_read24(p_ffd);
						RA8876_writeData(u32_pixel);
						RA8876_writeData(u32_pixel >> 8);
						RA8876_writeData(u32_pixel >> 16);
					}
				}
			}
		}
	}
}

void RAHelper_loadBitmapFromPath(uint32_t layerAddr, uint16_t x0, uint16_t y0, const char *str_imagePath)
{
	FIL		ffd;
	FRESULT fr;
	
	fr = f_open(&ffd, str_imagePath, FA_READ);
	
	if (fr == FR_OK)
	{
		f_lseek(&ffd, 0);
#ifdef GESIBUG
		gs_log_f(GLOG_INFO, "Lade Datei %s", str_imagePath);
#endif
		RAHelper_loadBitmapFromFIL(layerAddr, x0, y0, &ffd);
	}
	else
	{
#ifdef GESIBUG
		gs_log_f(GLOG_ERROR, "Fehler beim Öffnen der Datei! Code %d\r\nPfad: %s", fr, str_imagePath);
#endif
		RA8876_setTextCoords(x0, y0);
		RA8876_print_PROGMEM("Bild Ladefehler!");
	}
	f_close(&ffd);
}

void RAHelper_displayBitmapFromFlash(uint32_t dispAddr,uint16_t x0, uint16_t y0, uint32_t flashAddr, uint16_t width, uint16_t height)
{
	RA8876_spi_DMA_mode();
	RA8876_spi_selectFlash(1);
	RA8876_spi_DMA_flashAddress(flashAddr);
	RA8876_spi_DMA_Dest_Coords(x0, y0);
	RA8876_spi_DMA_WindowSize(width,height);
	RA8876_spi_DMA_SrcWidth(1024);
	
	RA8876_spi_DMA_start();
	// Wait until drawing is done
	while(RA8876_readStatus() & RA8876_CORE_BUSY);
}