/*
 * pifdec.c

 * PIF Decoder Library
 * Copyright (c) 2022 gfcwfzkm ( gfcwfzkm@protonmail.com )
 * License: GNU Lesser General Public License, Version 2.1
 * 		http://www.gnu.org/licenses/lgpl-2.1.html
 *
 * Created: 01.04.2022
 *  Author: gfcwfzkm
 */ 

#include "pifdec.h"

#if defined(AVR) && !defined(__GNUG__)
	#include <avr/pgmspace.h>
	#define _PMEMX	__memx
	#define _PRGM
#elif defined(AVR) && defined(__GNUG__)
	#include <avr/pgmspace.h>
	#define ARDUINO_IN_USE
	#define _PMEMX 
	#define _PRGM	PROGMEM
#else
	#define _PMEMX
	#define _PRGM
#endif


// CGA / 16 Color palette generated with the following formula:
// red	 = 255 * (2/3 * (colorNumber & 4)/4 + 1/3 * (colorNumber & 8)/8 )
// green = 255 * (2/3 * (colorNumber & 2)/2 + 1/3 * (colorNumber & 8)/8 )
// blue	 = 255 * (2/3 * (colorNumber & 1)/1 + 1/3 * (colorNumber & 8)/8 )
#if defined(PIF_RGB16C_RGB888)
const _PMEMX uint8_t color_table_16C[48] _PRGM = {
	0, 0, 0,		// black
	0, 0, 170,		// blue
	0, 170, 0,		// green
	0, 170, 170,	// cyan
	170, 0, 0,		// red
	170, 0, 170,	// magenta
	170, 170, 0,	// dark yellow
	170, 170, 170,	// light gray
	85, 85, 85,		// dark gray
	85, 85, 255,	// light blue
	85, 255, 85,	// light green
	85, 255, 255,	// light cyan
	255, 85, 85,	// light red
	255, 85, 255,	// light magenta
	255, 255, 85,	// yellow
	255, 255, 255	// white
};
#elif defined(PIF_RGB16C_RGB565)
const _PMEMX uint8_t color_table_16C[32] _PRGM = {
	0x00, 0x00,	// black
	0x00, 0x15,	// blue
	0x05, 0x40,	// green
	0x05, 0x55,	// cyan
	0xA8, 0x0,	// red
	0xA8, 0x15,	// magenta
	0xAD, 0x40,	// dark yellow
	0xAD, 0x55,	// light gray
	0x52, 0xAA,	// dark gray
	0x52, 0xBF,	// light blue
	0x57, 0xEA,	// light green
	0x57, 0xFF,	// light cyan
	0xFA, 0xAA,	// light red
	0xFA, 0xBF,	// light magenta
	0xFF, 0xEA,	// yellow
	0xFF, 0xFF	// white
};
#elif defined(PIF_RGB16C_RGB332)
const _PMEMX uint8_t color_table_16C[16] _PRGM = {
	0x00,	// black
	0x02,	// blue
	0x14,	// green
	0x16,	// cyan
	0xA0,	// red
	0xA2,	// magenta
	0xB4,	// dark yellow
	0xB6,	// light gray
	0x49,	// dark gray
	0x4B,	// light blue
	0x5D,	// light green
	0x5F,	// light cyan
	0xF5,	// light red
	0xEB,	// light magenta
	0xFD,	// yellow
	0xFF	// white
};
#else
	#error "You need to configure which color-format you want to use for the RGB16C image mode!"
#endif

#define PIF_MONOCHROME_BLACK	0x000000
#define PIF_MONOCHROME_WHITE	0xFFFFFF

#define PIF_FORMAT_HEADER	0x00464950	// 'PIF\0' as String in LittleEndian
#define PIF_FORMAT_RGB888	0x433C
#define PIF_FORMAT_RGB565	0xE5C5
#define PIF_FORMAT_RGB332	0x1E53
#define PIF_FORMAT_RGB16C	0xB895
#define PIF_FORMAT_BW		0x7DAA
#define PIF_FORMAT_IND24	0x4952
#define PIF_FORMAT_IND16	0x4947
#define PIF_FORMAT_IND8		0x4942
#define PIF_FORMAT_COMPR	0x7DDE
#define PIF_FORMAT_COLORTABLE_OFFSET	0x1C
#define PIF_MASK_INDEXED_COLOR_SIZE		0x03
#define PIF_MASK_INDEXED_MODE_IN_USE	0x04

/* Read data at various sizes, making sure the right endian is used */
uint8_t _read8(pifIO_t *p_io)
{
	uint8_t data8;
	
	p_io->readByte(p_io->fileHandle, &data8, 1);
	
	return data8;
}

uint16_t _read16(pifIO_t *p_io)
{
	uint8_t data8[2];
	
	p_io->readByte(p_io->fileHandle, data8, 2);
	
	return (uint16_t)data8[1] << 8 | data8[0];
}

// __uint24 is supported only on some plattforms but not on all of them, so treat it as u32
uint32_t _read24(pifIO_t *p_io)
{
	uint8_t data8[3];
	
	p_io->readByte(p_io->fileHandle, data8, 3);
	
	return (uint32_t)data8[2] << 16 | (uint32_t)data8[1] << 8 | data8[0];
}

uint32_t _read32(pifIO_t *p_io)
{
	uint8_t data8[4];
	
	p_io->readByte(p_io->fileHandle, data8, 4);
	
	return (uint32_t)data8[3] << 24 | (uint32_t)data8[2] << 16 | (uint32_t)data8[1] << 8 | data8[0];
}

/* Read the static color table for BW / RGB16C */
static inline uint32_t _getRGB16C(uint8_t color)
{
#if defined(ARDUINO_IN_USE)
	#warning "Arduino static indexed colors not fully tested - be aware!"
	#if defined(PIF_RGB16C_RGB888)
		return ((uint32_t)pgm_read_dword(&(color_table_16C[color * 3])) << 16) | ((uint32_t)pgm_read_dword(&(color_table_16C[color * 3 + 1))] << 8) | (pgm_read_dword(&(color_table_16C[color * 3 + 2])));
	#elif defined(PIF_RGB16C_RGB565)
		return ((uint32_t)pgm_read_dword(&(color_table_16C[color * 2])) << 8) | ((uint32_t)pgm_read_dword(&(color_table_16C[color * 2 + 1])));
	#elif defined(PIF_RGB16C_RGB332)
		return ((uint32_t)pgm_read_dword(&(color_table_16C[color])));
	#else
		#error "You need to configure which color-format you want to use for the RGB16C image mode!"
	#endif
#else
	#if defined(PIF_RGB16C_RGB888)
		return ((uint32_t)color_table_16C[color * 3] << 16) | ((uint32_t)color_table_16C[color * 3 + 1] << 8) | (color_table_16C[color * 3 + 2]);
	#elif defined(PIF_RGB16C_RGB565)
		return ((uint32_t)color_table_16C[color * 2] << 8) | ((uint32_t)color_table_16C[color * 2 + 1]);
	#elif defined(PIF_RGB16C_RGB332)
		return ((uint32_t)color_table_16C[color]);
	#else
		#error "You need to configure which color-format you want to use for the RGB16C image mode!"
	#endif
#endif
}

/* Read the indexed color either from the buffer or from the file */
static inline uint32_t _getIndexedColor(uint8_t color, pifHANDLE_t *p_pif, uint8_t *seekUsed)
{
	uint8_t mult = p_pif->pifInfo.imageType & PIF_MASK_INDEXED_COLOR_SIZE;
	uint32_t pixelColor;
	
	// If any colors have been loaded, use the buffered color table, otherwise seek back and forth (slow operation!)
	if (((color+1) * mult) <= p_pif->pifDecoder->colTableBufLen)
	{
		pixelColor = p_pif->pifDecoder->colTableBuf[mult * color];
		if (mult > 1)	pixelColor |= (uint32_t)p_pif->pifDecoder->colTableBuf[mult * color + 1] << 8;
		if (mult > 2)	pixelColor |= (uint32_t)p_pif->pifDecoder->colTableBuf[mult * color + 2] << 16;
	}
	else
	{
		*seekUsed = 1;
		p_pif->pifFileHandler->seekPos(p_pif->pifFileHandler->fileHandle, PIF_FORMAT_COLORTABLE_OFFSET + (mult * color));
		if (p_pif->pifInfo.imageType == PIF_TYPE_IND8)
		{
			pixelColor = _read8(p_pif->pifFileHandler);
		}
		else if (p_pif->pifInfo.imageType == PIF_TYPE_IND16)
		{
			pixelColor = _read16(p_pif->pifFileHandler);
		}
		else
		{
			pixelColor = _read24(p_pif->pifFileHandler);
		}
	}
	
	return pixelColor;
}

/* Process the indexed image by looking up the color table */
uint8_t _processIndexed(pifHANDLE_t *p_pif, uint8_t pixelGroup)
{
	uint8_t seek_used = 0;
	uint8_t pixelCounter = 0;
	uint8_t const bitsPerPixel = (p_pif->pifInfo.bitsPerPixel == 3) ? 4 : p_pif->pifInfo.bitsPerPixel;
	uint8_t const pixelLimit = 8 / bitsPerPixel;
	uint8_t const pixelMask = (1 << p_pif->pifInfo.bitsPerPixel) - 1;
	
	// Process the bits, that are packed within a byte and look up it's color
	for (;pixelCounter < pixelLimit; pixelCounter++)
	{
		// If the color table is to be bypassed, send the raw value straight to the drawing function,
		// otherwise look it up according to the image format
		if (p_pif->pifDecoder->bypassColTable == PIF_INDEXED_NORMAL_OPERATION)
		{
			switch (p_pif->pifInfo.imageType)
			{
				case PIF_TYPE_RGB16C:
					p_pif->pifDecoder->draw(p_pif->pifDecoder->displayHandle, &(p_pif->pifInfo), _getRGB16C(pixelGroup & 0x0F));
					break;
				case PIF_TYPE_BW:
					p_pif->pifDecoder->draw(p_pif->pifDecoder->displayHandle, &(p_pif->pifInfo), (pixelGroup & 1) ? _getRGB16C(15) : _getRGB16C(0));
					break;
				default:
					p_pif->pifDecoder->draw(p_pif->pifDecoder->displayHandle, &(p_pif->pifInfo), _getIndexedColor(pixelGroup & pixelMask, p_pif, &seek_used));
					break;
			}
		}
		else
		{
			p_pif->pifDecoder->draw(p_pif->pifDecoder->displayHandle, &(p_pif->pifInfo), pixelGroup & pixelMask);
		}
		
		// Cycle to the next bitgroup that represents a pixel
		// if none is left to process within the byte, return
		pixelGroup >>= bitsPerPixel;
		
		if (pixelCounter >= 1)
		{
			p_pif->pifInfo.currentX++;
			if (p_pif->pifInfo.currentX >= p_pif->pifInfo.imageWidth)
			{
				p_pif->pifInfo.currentX = 0;
				p_pif->pifInfo.currentY++;
				if (p_pif->pifInfo.currentY >= p_pif->pifInfo.imageHeight)
				{
					break;
				}
			}
		}
	}
	return seek_used;
}

pifRESULT pif_createPainter(pifPAINT_t *p_painter, PIF_PREPARE_IMAGE *f_optional_prepare, PIF_DRAW_PIXEL *f_draw, PIF_FINISH_IMAGE *f_optional_finish, void *p_displayHandler, uint8_t *p8_opt_ColTableBuf, uint16_t u16_colTableBufLength)
{
	p_painter->prepare = f_optional_prepare;
	p_painter->draw = f_draw;
	p_painter->finish = f_optional_finish;
	p_painter->displayHandle = p_displayHandler;
	p_painter->colTableBuf = p8_opt_ColTableBuf;
	p_painter->colTableBufLen = u16_colTableBufLength;
	return (f_draw == NULL) ? PIF_RESULT_DRAWERR : PIF_RESULT_OK;
}

pifRESULT pif_createIO(pifIO_t *p_fileIO, PIF_OPEN_FILE *f_openFile, PIF_CLOSE_FILE *f_closeFile, PIF_READ_FILE *f_readFile, PIF_SEEK_FILE *f_seekFile)
{
	p_fileIO->open = f_openFile;
	p_fileIO->close = f_closeFile;
	p_fileIO->readByte = f_readFile;
	p_fileIO->seekPos = f_seekFile;
	if ((f_openFile == NULL) || (f_readFile == NULL) || (f_seekFile == NULL))
	{
		return PIF_RESULT_IOERR;
	}
	else
	{
		return PIF_RESULT_OK;
	}
}

void pif_createPIFHandle(pifHANDLE_t *p_PIF, pifIO_t *p_fileIO, pifPAINT_t *p_painter)
{
	p_PIF->pifDecoder = p_painter;
	p_PIF->pifFileHandler = p_fileIO;
}

// Not only open the image file, but also analyse it and store the information
pifRESULT pif_open(pifHANDLE_t *p_PIF, const char *pc_path)
{
	int8_t results;
	uint16_t tempVar;
	
	// Open file and check for errors. If there is an error, cancel operation!
	p_PIF->pifFileHandler->fileHandle = p_PIF->pifFileHandler->open(pc_path, &results);
	if ((results != 0) || (p_PIF->pifFileHandler->readByte == NULL))
	{
		if (p_PIF->pifFileHandler->close != NULL && p_PIF->pifFileHandler->fileHandle != NULL)	p_PIF->pifFileHandler->close(p_PIF->pifFileHandler->fileHandle);
		return PIF_RESULT_IOERR;
	}
	
	// Interpret the PIF image header
	if (_read32(p_PIF->pifFileHandler) != PIF_FORMAT_HEADER)
	{
		// Not the file we expected!
		p_PIF->pifFileHandler->close(&(p_PIF->pifFileHandler->fileHandle));	
		return PIF_RESULT_FORMATERR;
	}
	
	p_PIF->pifInfo.fileSize = _read32(p_PIF->pifFileHandler);
	p_PIF->pifInfo.imageOffset = _read32(p_PIF->pifFileHandler);
	
	tempVar = _read16(p_PIF->pifFileHandler);
	switch(tempVar)
	{
		case PIF_FORMAT_RGB888:
			p_PIF->pifInfo.imageType = PIF_TYPE_RGB888;
			break;
		case PIF_FORMAT_RGB565:
			p_PIF->pifInfo.imageType = PIF_TYPE_RGB565;
			break;
		case PIF_FORMAT_RGB332:
			p_PIF->pifInfo.imageType = PIF_TYPE_RGB332;
			break;
		case PIF_FORMAT_RGB16C:
			p_PIF->pifInfo.imageType = PIF_TYPE_RGB16C;
			break;
		case PIF_FORMAT_BW:
			p_PIF->pifInfo.imageType = PIF_TYPE_BW;
			break;
		case PIF_FORMAT_IND24:
			p_PIF->pifInfo.imageType = PIF_TYPE_IND24;
			break;
		case PIF_FORMAT_IND16:
			p_PIF->pifInfo.imageType = PIF_TYPE_IND16;
			break;
		case PIF_FORMAT_IND8:
			p_PIF->pifInfo.imageType = PIF_TYPE_IND8;
			break;
		default:
			// Unsupported image type
			results |= 1;
	}
	p_PIF->pifInfo.bitsPerPixel = _read16(p_PIF->pifFileHandler);
	p_PIF->pifInfo.imageWidth = _read16(p_PIF->pifFileHandler);
	p_PIF->pifInfo.imageHeight = _read16(p_PIF->pifFileHandler);
	p_PIF->pifInfo.imageSize = _read32(p_PIF->pifFileHandler);
	p_PIF->pifInfo.colTableSize = _read16(p_PIF->pifFileHandler);
	
	tempVar = _read16(p_PIF->pifFileHandler);
	if (tempVar == PIF_FORMAT_COMPR)
	{
		p_PIF->pifInfo.compression = PIF_COMPRESSION_RLE;
	}
	else if (tempVar == 0)
	{
		p_PIF->pifInfo.compression = PIF_COMPRESSION_NONE;
	}
	else
	{
		// Unsupported compression
		results |= 1;
	}
	
	p_PIF->pifInfo.startX = 0;
	p_PIF->pifInfo.startY = 0;
	p_PIF->pifInfo.currentX = 0;
	p_PIF->pifInfo.currentY = 0;
	
	if (results)
	{
		if (p_PIF->pifFileHandler->close != NULL) p_PIF->pifFileHandler->close(&(p_PIF->pifFileHandler->fileHandle));
		return PIF_RESULT_FORMATERR;
	}
	return PIF_RESULT_OK;
}

pifRESULT pif_display(pifHANDLE_t *p_PIF, uint16_t x0, uint16_t y0)
{
	int8_t rleInstr = 0;
	uint32_t pixelData;
	uint8_t seekModified = 0;
	
	const uint8_t ColorTablePixelSize = p_PIF->pifInfo.imageType & PIF_MASK_INDEXED_COLOR_SIZE;
	const uint8_t filePosInc = (p_PIF->pifInfo.bitsPerPixel < 8) ? 1 : p_PIF->pifInfo.bitsPerPixel >> 3; // Division by 8
	
	p_PIF->pifFileHandler->filePos = 0;
	p_PIF->pifInfo.startX = x0;
	p_PIF->pifInfo.startY = y0;
	
	// If function pointer != null, call it with the image details
	if (p_PIF->pifDecoder->prepare != NULL)
	{
		if (p_PIF->pifDecoder->prepare(p_PIF->pifDecoder->displayHandle, &(p_PIF->pifInfo)))
		{
			return PIF_RESULT_DRAWERR;
		}
	}
	
	// Usually 0x1C is the image data offset address, unless a color table is in use
	if ((p_PIF->pifInfo.imageType & PIF_MASK_INDEXED_MODE_IN_USE) && (p_PIF->pifDecoder->bypassColTable == PIF_INDEXED_NORMAL_OPERATION))
	{		
		// Buffer some colors from the color table, if there is any buffer available
		if (p_PIF->pifDecoder->colTableBuf != NULL && p_PIF->pifDecoder->colTableBufLen >= ColorTablePixelSize)
		{
			// Allow partial buffering by only buffer the first x colors that the array can fit in
			// Only buffer whole colors (RGB332, RGB565 or RGB888), not partially (clipping RGB888 into a 2-Byte buffer, for example)
			for (uint8_t colorByteCnt = 0; (colorByteCnt + ColorTablePixelSize - 1) < p_PIF->pifDecoder->colTableBufLen; colorByteCnt++)
			{
				if (colorByteCnt >= (p_PIF->pifInfo.colTableSize))
				{
					// No more colors to read from the color table
					break;
				}
				p_PIF->pifDecoder->colTableBuf[colorByteCnt] = _read8(p_PIF->pifFileHandler);
			}
		}
	}
	// Seek to the right position for the image data
	p_PIF->pifFileHandler->seekPos(p_PIF->pifFileHandler->fileHandle, p_PIF->pifInfo.imageOffset);
	
	// Check if data is RLE-compressed
	if (p_PIF->pifInfo.compression == PIF_COMPRESSION_RLE)
	{
		for (; p_PIF->pifFileHandler->filePos < p_PIF->pifInfo.imageSize; p_PIF->pifFileHandler->filePos++)
		{
			// Load the next byte			
			pixelData = _read8(p_PIF->pifFileHandler);
			
			// Check the RLE Instruction
			if (rleInstr > 0)
			{
				// RLE Instruction is positive: Send the pixel rleInst-amount of times
				// Load additional bytes if RGB565 or RGB888 is used
				if (p_PIF->pifInfo.bitsPerPixel > 16)
				{
					pixelData |= (uint32_t)_read16(p_PIF->pifFileHandler) << 8;
					p_PIF->pifFileHandler->filePos += 2;
				}
				else if (p_PIF->pifInfo.bitsPerPixel > 8)
				{
					pixelData |= (uint32_t)_read8(p_PIF->pifFileHandler) << 8;
					p_PIF->pifFileHandler->filePos++;
				}
				
				for (; rleInstr > 0; rleInstr--)
				{
					
					if (p_PIF->pifInfo.imageType <= PIF_TYPE_RGB332)
					{
						// raw image
						p_PIF->pifDecoder->draw(p_PIF->pifDecoder->displayHandle, &(p_PIF->pifInfo), pixelData);
					}
					else
					{
						// indexed image
						seekModified = _processIndexed(p_PIF, pixelData);
					}
					// Increase Pixel Position counter
					p_PIF->pifInfo.currentX++;
					if (p_PIF->pifInfo.currentX >= p_PIF->pifInfo.imageWidth)
					{
						p_PIF->pifInfo.currentX = 0;
						p_PIF->pifInfo.currentY++;
					}
				}
			}
			else if (rleInstr < 0)
			{
				// RLE Instruction is negative: The next (rleInst * -1)-amount of image pixels are uncompressed
				// Load additional bytes if RGB565 or RGB888 is used
				if (p_PIF->pifInfo.bitsPerPixel > 16)
				{
					pixelData |= (uint32_t)_read16(p_PIF->pifFileHandler) << 8;
					p_PIF->pifFileHandler->filePos += 2;
				}
				else if (p_PIF->pifInfo.bitsPerPixel > 8)
				{
					pixelData |= (uint32_t)_read8(p_PIF->pifFileHandler) << 8;
					p_PIF->pifFileHandler->filePos++;
				}
				if (p_PIF->pifInfo.imageType <= PIF_TYPE_RGB332)
				{
					// raw image
					p_PIF->pifDecoder->draw(p_PIF->pifDecoder->displayHandle, &(p_PIF->pifInfo), pixelData);
				}
				else
				{
					// indexed image
					seekModified = _processIndexed(p_PIF, pixelData);
				}
				// Increase Pixel Position counter
				p_PIF->pifInfo.currentX++;
				if (p_PIF->pifInfo.currentX >= p_PIF->pifInfo.imageWidth)
				{
					p_PIF->pifInfo.currentX = 0;
					p_PIF->pifInfo.currentY++;
				}
				rleInstr++;
			}
			else
			{
				// RLE Instruction is zero / empty - load the next RLE instruction
				rleInstr = (int8_t)pixelData;
			}
			
			if (seekModified)
			{
				// If the file position has been modified by the indexed-image function, restore the index
				p_PIF->pifFileHandler->seekPos(p_PIF->pifFileHandler->fileHandle, p_PIF->pifFileHandler->filePos + p_PIF->pifInfo.imageOffset + 1);
			}
		}
	}
	else
	{
		for (p_PIF->pifInfo.currentY = 0; p_PIF->pifInfo.currentY < p_PIF->pifInfo.imageHeight; p_PIF->pifInfo.currentY++)
		{
			for (p_PIF->pifInfo.currentX = 0; p_PIF->pifInfo.currentX < p_PIF->pifInfo.imageWidth; p_PIF->pifInfo.currentX++)
			{
				p_PIF->pifFileHandler->filePos += filePosInc;
				if (p_PIF->pifInfo.bitsPerPixel > 16)
				{
					pixelData = _read24(p_PIF->pifFileHandler);
				}
				else if (p_PIF->pifInfo.bitsPerPixel > 8)
				{
					pixelData = _read16(p_PIF->pifFileHandler);
				}
				else
				{
					pixelData = _read8(p_PIF->pifFileHandler);
				}
								
				if (p_PIF->pifInfo.imageType <= PIF_TYPE_RGB332)
				{
					// Raw RGB888 / RGB565 / RGB332 image data
					p_PIF->pifDecoder->draw(p_PIF->pifDecoder->displayHandle, &(p_PIF->pifInfo), pixelData);
				}
				else
				{
					// Treat non-RGB888 / RGB565 / RGB332 image data as indexed
					_processIndexed(p_PIF, pixelData);
				}
			}
		}
		
	}
	
	// If function pointer != zero, call it
	if (p_PIF->pifDecoder->finish != NULL)
	{
		if (p_PIF->pifDecoder->finish(p_PIF->pifDecoder->displayHandle, &(p_PIF->pifInfo)))	return PIF_RESULT_DRAWERR;
	}
	return PIF_RESULT_OK;
}

pifRESULT pif_getInfo(pifHANDLE_t *p_PIF, pifINFO_t *p_Info)
{
	p_Info = &(p_PIF->pifInfo);
	return PIF_RESULT_OK;
}

pifRESULT pif_close(pifHANDLE_t *p_PIF)
{
	if (p_PIF->pifFileHandler->close != NULL)
	{
		if (p_PIF->pifFileHandler->close(p_PIF->pifFileHandler->fileHandle))
		{
			return PIF_RESULT_IOERR;
		}
	}
	return PIF_RESULT_OK;
}

pifRESULT pif_OpenAndDisplay(pifHANDLE_t *p_PIF, const char *pc_path, uint16_t x0, uint16_t y0)
{
	pifRESULT result = PIF_RESULT_OK;
	result = pif_open(p_PIF, pc_path);
	if (result != PIF_RESULT_OK)	return result;
	result = pif_display(p_PIF, x0, y0);
	if (result != PIF_RESULT_OK)	return result;
	result = pif_close(p_PIF);
	return result;
}

// Color converter Function for the user-implemented driver using integer math
// Converting from any mode to any mode
// Converting from RGB888 to RGB565/RGB332 or RGB565 to RGB332: Simple mask conversion
// Possible way to convert RGB332 to RGB888:
// red = 36*r + 3
// green = 36*g + 3
// blue = 85 * b
// Possible way to convert RGB332 to RGB565:
// red = 4*r + 3
// green = 9*g
// blue = 30*b
// Possible way to convert RGB565 to RGB888:
// red = 8*r + 4
// green = 4*g + 3
// blue = 8*b + 3
// Alternatively a "fast" mode is supported, using only bitshift instructions
uint32_t convertColor(uint32_t color, pifImageType sourceType, pifImageType targetType, pifColorConversion convMode)
{
	uint32_t outColor = 0;

	switch (targetType)
	{
		case PIF_TYPE_RGB888:
		case PIF_TYPE_IND24:
			switch (sourceType)
			{
				case PIF_TYPE_RGB332:
				case PIF_TYPE_IND8:
					if (convMode == PIF_CONV_ACCURATE)
					{
						outColor = (((color & 0xE0) >> 5) * 36 + 3) << 16;	// red
						outColor |= (((color & 0x1C) >> 2) * 36 + 3) << 8;	// green
						outColor |= ((color & 0x03) * 85);					// blue
					}
					else
					{
						outColor = (((color & 0xE0) >> 5) << 5) << 16;	// red
						outColor |= (((color & 0x1C) >> 2) << 5) << 8;	// green
						outColor |= ((color & 0x03) << 6);				// blue
					}
					break;
				case PIF_TYPE_RGB565:
				case PIF_TYPE_IND16:
					if (convMode == PIF_CONV_ACCURATE)
					{
						outColor = (((color & 0xF800) >> 11) * 8 + 4) << 16;// red
						outColor |= (((color & 0x07E0) >> 5) * 4 + 3) << 8;	// green
						outColor |= ((color & 0x001F) * 8 + 3);				// blue
					}
					else
					{
						outColor = (((color & 0xF800) >> 11) << 3) << 16;	// red
						outColor |= (((color & 0x07E0) >> 5) << 2) << 8;	// green
						outColor |= ((color & 0x001F) << 3);				// blue
					}
					break;
				default:
					// Nothing to do
					break;
			}
			break;
		case PIF_TYPE_RGB565:
		case PIF_TYPE_IND16:
			switch (sourceType)
			{
				case PIF_TYPE_RGB888:
				case PIF_TYPE_IND24:
					outColor = (color & 0xF80000) >> 8;		// red
					outColor |= (color & 0x00FC00) >> 5;	// green
					outColor |= (color & 0x0000F8) >> 3;	// blue
					break;
				case PIF_TYPE_RGB332:
				case PIF_TYPE_IND8:
					if (convMode == PIF_CONV_ACCURATE)
					{
						outColor = (((color & 0xE0) >> 5) * 4 + 3) << 11;	// red
						outColor |= (((color & 0x1C) >> 2) * 9) << 5;		// green
						outColor |= ((color & 0x03) * 10);					// blue
					}
					else
					{
						outColor = (((color & 0xE0) >> 5) << 2) << 11;		// red
						outColor |= (((color & 0x1C) >> 2) << 3) << 5;		// green
						outColor |= ((color & 0x03) << 3);					// blue
					}
					break;
				default:
					// Nothing to do
					break;
			}
			break;
		case PIF_TYPE_RGB332:
		case PIF_TYPE_IND8:
			switch (sourceType)
			{
				case PIF_TYPE_RGB888:
				case PIF_TYPE_IND24:
					outColor = (color & 0xE00000) >> 16;	// red
					outColor |= (color & 0x00E000) >> 11;	// green
					outColor |= (color & 0x0000C0) >> 6;	// blue
					break;
				case PIF_TYPE_RGB565:
				case PIF_TYPE_IND16:
					outColor = (color & 0xE000) >> 8;		// red
					outColor |= (color & 0x0700) >> 6;		// green
					outColor |= (color & 0x0018) >> 3;		// blue
					break;
				default:
					// Nothing to do
					break;
			}
			break;
		default:
			// BW and RGB16 Conversion not supported, as the user
			// can configure the fitting pixel type in the header
            break;
	}

	return outColor;
}