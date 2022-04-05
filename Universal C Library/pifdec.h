/*
 * pifdec.h
 *
 * Created: 01.04.2022
 *  Author: gfcwfzkm
 */ 


#ifndef PIFDEC_H_
#define PIFDEC_H_

#include <inttypes.h>

/* Select the RGB16 Color Table Format */
//#define	PIF_RGB16C_RGB888
#define PIF_RGB16C_RGB565
//#define PIF_RGB16C_RGB332

typedef enum {
	PIF_RESULT_OK,
	PIF_RESULT_IOERR,
	PIF_RESULT_DRAWERR,
	PIF_RESULT_FORMATERR
}pifRESULT;

/*
// Todo: Configure the supported modes by the display, apply conversion
// within the library to the display's right format
typedef enum {
	PIF_DISPLAY_MONO	= 0x01,
	PIF_DISPLAY_RGB16C	= 0x02,
	PIF_DISPLAY_RGB332	= 0x04,
	PIF_DISPLAY_RGB565	= 0x08,
	PIF_DISPLAY_RGB888	= 0x10,
	PIF_DISPLAY_RGB		= 0x1C,
	PIF_DISPLAY_ALL		= 0x1F
}pifSupportedType;
*/
typedef enum {
	PIF_TYPE_RGB888 = 0,
	PIF_TYPE_RGB565 = 1,
	PIF_TYPE_RGB332 = 2,
	PIF_TYPE_RGB16C = 3,
	PIF_TYPE_MONO = 4,
	PIF_TYPE_IND8 = 5,
	PIF_TYPE_IND16 = 6,
	PIF_TYPE_IND24 = 7
}pifImageType;

typedef enum {
	PIF_COMPRESSION_NONE = 0,
	PIF_COMPRESSION_RLE
}pifCompression;

typedef struct {
	uint32_t fileSize;			// File Size of the Image
	uint32_t imageOffset;		// Index Offset to the image data array
	pifImageType imageType:4;	// Image Type
	uint16_t bitsPerPixel;		// BitsPerPixel
	uint16_t imageWidth;		// Image Width in Pixel
	uint16_t imageHeight;		// Image Height in Pixel
	uint32_t imageSize;			// Image Data Size in Bytes
	uint16_t colTableSize;		// Color Table Size in Entries
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
	uint8_t *colTableBuf;	// Optional array to buffer the color table
	uint16_t colTableBufLen;// Length of the color table buffer
}pifDEC_t;

// Callbacks are expected to return 0. Non-zero return is treatet as an error!
typedef void* (PIF_OPEN_FILE)(const char* pc_filePath, int8_t *fileError);
typedef int8_t (PIF_CLOSE_FILE)(void *p_fileHandle);
typedef void (PIF_READ_FILE)(void *p_fileHandle, uint8_t *p8_buf, uint8_t length);
typedef int8_t (PIF_SEEK_FILE)(void *p_fileHandle, uint32_t u32_filePos);

typedef struct {
	PIF_OPEN_FILE *open;
	PIF_CLOSE_FILE *close;
	PIF_READ_FILE *readByte;
	PIF_SEEK_FILE *seekPos;		// relevant for indexed image with no/small color table buffers.
	uint32_t filePos;			// File index, starting from the image data position (true index = filePos + imageOffset)
	void *fileHandle;			// File Handler
}pifIO_t;

typedef struct {
	pifINFO_t pifInfo;
	pifDEC_t *pifDecoder;
	pifIO_t *pifFileHandler;
}pifHANDLE_t;

void pif_createDecoder(pifDEC_t *p_decoder, PIF_PREPARE_IMAGE *f_optional_prepare, PIF_DRAW_PIXEL *f_draw, PIF_FINISH_IMAGE *f_optional_finish, void *p_displayHandler, uint8_t *p8_opt_ColTableBuf, uint16_t u16_colTableBufLength);
void pif_createIO(pifIO_t *p_fileIO, PIF_OPEN_FILE *f_openFile, PIF_CLOSE_FILE *f_closeFile, PIF_READ_FILE *f_readFile, PIF_SEEK_FILE *f_seekFile, void *p_fileHandler);
void pif_createPIFHandle(pifHANDLE_t *p_PIF, pifIO_t *p_fileIO, pifDEC_t *p_decoder);

pifRESULT pif_open(pifHANDLE_t *p_PIF, const char *pc_path);
pifRESULT pif_display(pifHANDLE_t *p_PIF, uint16_t x0, uint16_t y0);
pifRESULT pif_getInfo(pifHANDLE_t *p_PIF, pifINFO_t *p_Info);
pifRESULT pif_close(pifHANDLE_t *p_PIF);

// Combines pif_open, pif_display and pif_close in one call
pifRESULT pif_OpenAndDisplay(pifHANDLE_t *p_PIF, const char *pc_path, uint16_t x0, uint16_t y0);


#endif /* PIFDEC_H_ */