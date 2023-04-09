/**
 * @file pifdec.h
 *
 * @brief PIF Decoder Library
 * Copyright (c) 2022 gfcwfzkm ( gfcwfzkm@protonmail.com )
 * License: GNU Lesser General Public License, Version 2.1
 * 		http://www.gnu.org/licenses/lgpl-2.1.html
 *
 * @date	01.04.2022
 * @author	gfcwfzkm
 * @version	1
 * - First version with partial doxygen documentation
 */ 


#ifndef PIFDEC_H_
#define PIFDEC_H_

#include <inttypes.h>
#include <stddef.h>

#define PIF_VERSION_NUMBER	0x0001

/** Choose to embed a RGB888 lookup table for the RGB16C colors. */
//#define	PIF_RGB16C_RGB888
/** Choose to embed a RGB565 lookup table for the RGB16C colors */
#define PIF_RGB16C_RGB565
/** Choose to embed a RGB332 lookup table for the RGB16C colors */
//#define PIF_RGB16C_RGB332

/** States wether the operation was successful or if (and what) error occured */
typedef enum {
	PIF_RESULT_OK,			/**< Operation was successful */
	PIF_RESULT_IOERR,		/**<  I/O File error */
	PIF_RESULT_DRAWERR,		/**<  Drawing Routines error */
	PIF_RESULT_FORMATERR	/**<  PIF File Format error */
}pifRESULT;

/** Image Type of the .PIF image */
typedef enum {
	PIF_TYPE_RGB888 = 0,	/**< RGB888, 3 Bytes per Pixel */
	PIF_TYPE_RGB565 = 1,	/**< RGB565, 2 Bytes per Pixel */
	PIF_TYPE_RGB332 = 2,	/**< RGB332, 1 Byte per Pixel */
	PIF_TYPE_RGB16C = 3,	/**< RGB16C, 4 Bits per Pixel */
	PIF_TYPE_BW		= 4,	/**< BW, 1 Bit per Pixel */
	PIF_TYPE_IND8	= 5,	/**< IND8, 1 Byte per Color */
	PIF_TYPE_IND16	= 6,	/**< IND16, 2 Bytes per Color */
	PIF_TYPE_IND24	= 7		/**< IND24, 3 Bytes per Color */
}pifImageType;

/** Selects the conversion type from one to the other */
typedef enum {
	PIF_CONV_ACCURATE = 0,	/**< Accurate conversion */
	PIF_CONV_FAST = 1		/**< Fast conversion */
}pifColorConversion;

typedef enum {
	PIF_COMPRESSION_NONE = 0,	// No compression
	PIF_COMPRESSION_RLE			// RLE compression
}pifCompression;

typedef enum {
	PIF_INDEXED_NORMAL_OPERATION = 0,	// Normal operation
	PIF_INDEXED_BYPASS_LOOKUP = 1		// Ignore index table, send index value to user
}pifIndexedBypass;

typedef struct {
	uint32_t fileSize;			// File Size of the Image
	uint32_t imageOffset;		// Index Offset to the image data array
	pifImageType imageType:4;	// Image Type
	uint16_t bitsPerPixel;		// BitsPerPixel
	uint16_t imageWidth;		// Image Width in Pixel
	uint16_t imageHeight;		// Image Height in Pixel
	uint32_t imageSize;			// Image Data Size in Bytes
	uint16_t colTableSize;		// Color Table Size in Bytes
	pifCompression compression:4;// RLE Compression enabled or not
	uint16_t startX;			// Display Start Positon X
	uint16_t startY;			// Display Start Position Y
	uint16_t currentX;			// Current X Positon of the image being processed
	uint16_t currentY;			// Current Y Position of the image being processed
}pifINFO_t;

// Callbacks are expected to return 0. Non-zero return is treatet as an error!
typedef int8_t (PIF_PREPARE_IMAGE)(void *p_Display, pifINFO_t* p_pifInfo);
typedef void (PIF_DRAW_PIXEL)(void *p_Display, pifINFO_t* p_pifInfo, uint32_t pixel);
typedef int8_t (PIF_FINISH_IMAGE)(void *p_Display, pifINFO_t* p_pifInfo);

typedef struct {
	PIF_PREPARE_IMAGE *prepare;	// Optional Function to allow the display operation to be prepared
	PIF_DRAW_PIXEL *draw;		// Function to draw the pixel
	PIF_FINISH_IMAGE *finish;	// Optional Function to finish the display operation
	void *displayHandle;		// Display handler
	pifIndexedBypass bypassColTable;	// Bypass the lookup of indexed colors, handy for displays supporting very specific formats (like 7-colors e-ink displays)
	uint8_t *colTableBuf;		// Optional array to buffer the color table
	uint16_t colTableBufLen;	// Length of the color table buffer
}pifPAINT_t;

// FILE I/O FUNCTIONS REQUIRED FOR BASIC FUNCTIONALITY
// Calls with pointer to string and pointer to char. Expects a fileHandler pointer or a non-zero value in *fileError
typedef void* (PIF_OPEN_FILE)(const char* pc_filePath, int8_t *fileError);
// Calls with FileHandler pointer, expects the file to be closed
typedef int8_t (PIF_CLOSE_FILE)(void *p_fileHandle);
// Calls with FileHandle, pointer to array and length to be read.
typedef void (PIF_READ_FILE)(void *p_fileHandle, uint8_t *p8_buf, uint8_t length);
// Calls with FileHandle and new position to seek. Expects non-zero if the operation fails
typedef int8_t (PIF_SEEK_FILE)(void *p_fileHandle, uint32_t u32_filePos);

typedef struct {
	PIF_OPEN_FILE *open;		// Open the file
	PIF_CLOSE_FILE *close;		// Close the file
	PIF_READ_FILE *readByte;	// Read x amount of bytes
	PIF_SEEK_FILE *seekPos;		// relevant for indexed image with no/small color table buffers.
	uint32_t filePos;			// File index used internally - don't use in your I/O functions!
	void *fileHandle;			// File Handler used by the FILE I/O functions
}pifIO_t;

typedef struct {
	pifINFO_t pifInfo;		// Information about the (last) opened image
	pifPAINT_t *pifDecoder;	// Decoder to access the Display
	pifIO_t *pifFileHandler;// File Read Functions
}pifHANDLE_t;

pifRESULT pif_createPainter(pifPAINT_t *p_painter, PIF_PREPARE_IMAGE *f_optional_prepare, PIF_DRAW_PIXEL *f_draw, PIF_FINISH_IMAGE *f_optional_finish, void *p_displayHandler, uint8_t *p8_opt_ColTableBuf, uint16_t u16_colTableBufLength);
pifRESULT pif_createIO(pifIO_t *p_fileIO, PIF_OPEN_FILE *f_openFile, PIF_CLOSE_FILE *f_closeFile, PIF_READ_FILE *f_readFile, PIF_SEEK_FILE *f_seekFile);
void pif_createPIFHandle(pifHANDLE_t *p_PIF, pifIO_t *p_fileIO, pifPAINT_t *p_painter);

pifRESULT pif_open(pifHANDLE_t *p_PIF, const char *pc_path);
pifRESULT pif_display(pifHANDLE_t *p_PIF, uint16_t x0, uint16_t y0);
pifRESULT pif_getInfo(pifHANDLE_t *p_PIF, pifINFO_t *p_Info);
pifRESULT pif_close(pifHANDLE_t *p_PIF);

// Combines pif_open, pif_display and pif_close in one call
pifRESULT pif_OpenAndDisplay(pifHANDLE_t *p_PIF, const char *pc_path, uint16_t x0, uint16_t y0);

// Convert the pixel image data from one type to the other type as accurate as possible
// If possible, use images supported natively by the display - this is a helper function
// to provide image support for any display type. Setting convMode to 1 enables a faster conversion method at the
// cost of color accuracy.
uint32_t convertColor(uint32_t clor, pifImageType sourceType, pifImageType targetType, pifColorConversion convMode);

#endif /* PIFDEC_H_ */