/*
 * pifdec.c
 *
 * Created: 01.04.2022
 *  Author: gfcwfzkm
 */

#include "pifdec.h"

#ifdef AVR
	#include <avr/pgmspace.h>
	#define _PMEMX	__memx
#else
	#define _PMEMX
#endif

// CGA / 16 Color palette generated with the following formula:
// red	 = 2/3×(colorNumber & 4)/4 + 1/3×(colorNumber & 8)/8 * 255
// green = 2/3×(colorNumber & 2)/2 + 1/3×(colorNumber & 8)/8 * 255
// blue	 = 2/3×(colorNumber & 1)/1 + 1/3×(colorNumber & 8)/8 * 255
const _PMEMX uint8_t color_table_16C[48] = {
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

uint8_t _read8(pifIO_t *p_io, int8_t *result)
{
	uint8_t data8;
	
	*result |= p_io->readByte(p_io->fileHandle, &data8);
	
	return data8;
}

uint16_t _read16(pifIO_t *p_io, int8_t *result)
{
	uint8_t data8[2];
	
	*result |= p_io->readByte(p_io->fileHandle, &data8[0]);
	*result |= p_io->readByte(p_io->fileHandle, &data8[1]);
	
	return (uint16_t)data8[1] << 8 | data8[0];
}

// __uint24 is supported on some plattforms but not on all of them, so treat it as u32
uint32_t _read24(pifIO_t *p_io, int8_t *result)
{
	uint8_t data8[3];
	
	*result |= p_io->readByte(p_io->fileHandle, &data8[0]);
	*result |= p_io->readByte(p_io->fileHandle, &data8[1]);
	*result |= p_io->readByte(p_io->fileHandle, &data8[2]);
	
	return (uint32_t)data8[2] << 16 | (uint32_t)data8[1] << 8 | data8[0];
}

uint32_t _read32(pifIO_t *p_io, int8_t *result)
{
	uint8_t data8[4];
	
	*result |= p_io->readByte(p_io->fileHandle, &data8[0]);
	*result |= p_io->readByte(p_io->fileHandle, &data8[1]);
	*result |= p_io->readByte(p_io->fileHandle, &data8[2]);
	*result |= p_io->readByte(p_io->fileHandle, &data8[3]);
	
	return (uint32_t)data8[3] << 24 | (uint32_t)data8[2] << 16 | (uint32_t)data8[1] << 8 | data8[0];
}

void pif_createDecoder(pifDEC_t *p_decoder, PIF_PREPARE_IMAGE *f_optional_prepare, PIF_DRAW_PIXEL *f_draw, PIF_FINISH_IMAGE *f_optional_finish, void *p_displayHandler, uint8_t *p8_opt_ColTableBuf, uint16_t u16_colTableBufLength)
{
	p_decoder->prepare = f_optional_prepare;
	p_decoder->draw = f_draw;
	p_decoder->finish = f_optional_finish;
	p_decoder->displayHandle = p_displayHandler;
	p_decoder->colTableBuf = p8_opt_ColTableBuf;
	p_decoder->colTableBufLen = u16_colTableBufLength;
}

void pif_createIO(pifIO_t *p_fileIO, PIF_OPEN_FILE *f_openFile, PIF_CLOSE_FILE *f_closeFile, PIF_READ_FILE *f_readFile, PIF_SEEK_FILE *f_seekFile, void *p_fileHandler)
{
	p_fileIO->open = f_openFile;
	p_fileIO->close = f_closeFile;
	p_fileIO->readByte = f_readFile;
	p_fileIO->seekPos = f_seekFile;
	p_fileIO->fileHandle = p_fileHandler;
}

void pif_createPIFHandle(pifHANDLE_t *p_PIF, pifIO_t *p_fileIO, pifDEC_t *p_decoder)
{
	p_PIF->pifDecoder = p_decoder;
	p_PIF->pifFileHandler = p_fileIO;
}

// Not only open the image file, but also analyse it and store the information
pifRESULT pif_open(pifHANDLE_t *p_PIF, const char *pc_path)
{
	int8_t results;
	uint16_t tempVar;
	
	// Open file and check for errors. If there is an error, cancel operation!
	p_PIF->pifFileHandler->fileHandle = p_PIF->pifFileHandler->open(pc_path, &results);
	if (results != 0)
	{
		p_PIF->pifFileHandler->close(p_PIF->pifFileHandler->fileHandle);
		return PIF_RESULT_IOERR;
	}
	
	// Interpret the PIF image header
	if (_read32(p_PIF->pifFileHandler, &results) != PIF_FORMAT_HEADER)
	{
		// Not the file we expected!
		p_PIF->pifFileHandler->close(&(p_PIF->pifFileHandler->fileHandle));	
		return PIF_RESULT_FORMATERR;
	}
	
	p_PIF->pifInfo.fileSize = _read32(p_PIF->pifFileHandler, &results);
	p_PIF->pifInfo.imageOffset = _read32(p_PIF->pifFileHandler, &results);
	
	tempVar = _read16(p_PIF->pifFileHandler, &results);
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
			p_PIF->pifInfo.imageType = PIF_TYPE_MONO;
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
	p_PIF->pifInfo.bitsPerPixel = _read16(p_PIF->pifFileHandler, &results);
	p_PIF->pifInfo.imageWidth = _read16(p_PIF->pifFileHandler, &results);
	p_PIF->pifInfo.imageHeight = _read16(p_PIF->pifFileHandler, &results);
	p_PIF->pifInfo.imageSize = _read32(p_PIF->pifFileHandler, &results);
	p_PIF->pifInfo.colTableSize = _read16(p_PIF->pifFileHandler, &results);
	
	tempVar = _read16(p_PIF->pifFileHandler, &results);
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
		p_PIF->pifFileHandler->close(&(p_PIF->pifFileHandler->fileHandle));
		return PIF_RESULT_FORMATERR;
	}
	return PIF_RESULT_OK;
}

pifRESULT pif_display(pifHANDLE_t *p_PIF, uint16_t x0, uint16_t y0)
{
	int8_t results = 0;
	uint32_t pixelData;
	
	p_PIF->pifInfo.startX = x0;
	p_PIF->pifInfo.startY = y0;
	
	// If function pointer != zero, call it
	if (p_PIF->pifDecoder->prepare != 0)
	{
		if (p_PIF->pifDecoder->prepare(p_PIF->pifDecoder->displayHandle, &(p_PIF->pifInfo)))
		{
			return PIF_RESULT_DRAWERR;
		}
	}
	
	if (p_PIF->pifDecoder->colTableBufLen > 0 && p_PIF->pifInfo.imageType >= PIF_TYPE_IND24)
	{
		// Buffer some colors from the color table
		// Allow partial buffering by only buffer the first x colors that the array can fit in
	}
	
	// Check if data is RLE-compressed
	if (p_PIF->pifInfo.compression == PIF_COMPRESSION_RLE)
	{
		
	}
	else
	{
		for (p_PIF->pifInfo.currentY = 0; p_PIF->pifInfo.currentY < p_PIF->pifInfo.imageHeight; p_PIF->pifInfo.currentY++)
		{
			for (p_PIF->pifInfo.currentX = 0; p_PIF->pifInfo.currentX < p_PIF->pifInfo.imageWidth; p_PIF->pifInfo.currentX++)
			{
				if (p_PIF->pifInfo.imageType <= PIF_TYPE_RGB332)
				{
					pixelData = (uint32_t)_read8(p_PIF->pifFileHandler, &results);
					if (p_PIF->pifInfo.imageType <= PIF_TYPE_RGB565)
						pixelData |= (uint32_t)_read8(p_PIF->pifFileHandler, &results) << 8;
					if (p_PIF->pifInfo.imageType == PIF_TYPE_RGB888)
						pixelData |= (uint32_t)_read8(p_PIF->pifFileHandler, &results) << 16;
					p_PIF->pifDecoder->draw(p_PIF->pifDecoder->displayHandle, &(p_PIF->pifInfo), pixelData);
				}
				else
				{
					// Treat non-RGB888 / RGB565 / RGB332 image data as indexed
				}
			}
		}
	}
	
	// If function pointer != zero, call it
	if (p_PIF->pifDecoder->finish != 0)
	{
		results |= p_PIF->pifDecoder->finish(p_PIF->pifDecoder->displayHandle, &(p_PIF->pifInfo));
	}
	
	if (results)
	{
		return PIF_RESULT_DRAWERR;
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
	if (p_PIF->pifFileHandler->close(&(p_PIF->pifFileHandler->fileHandle)))	return PIF_RESULT_IOERR;
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