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
 * @version	2
 * - Doxygen documentation added to the header file
 * @version 1
 */ 

#ifndef PIFDEC_H_
#define PIFDEC_H_

#include <inttypes.h>
#include <stddef.h>

/** Incremental 16bit versioning number*/
#define PIF_VERSION_NUMBER	0x0002

/** Choose to embed a RGB888 lookup table for the RGB16C colors. */
//#define	PIF_RGB16C_RGB888
/** Choose to embed a RGB565 lookup table for the RGB16C colors */
#define PIF_RGB16C_RGB565
/** Choose to embed a RGB332 lookup table for the RGB16C colors */
//#define PIF_RGB16C_RGB332

/** States wether the operation was successful or if (and what) error occured */
typedef enum {
	PIF_RESULT_OK,			/**< Operation was successful */
	PIF_RESULT_IOERR,		/**< I/O File error */
	PIF_RESULT_DRAWERR,		/**< Drawing Routines error */
	PIF_RESULT_FORMATERR	/**< PIF File Format error */
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

/** Compression type of the current open file */
typedef enum {
	PIF_COMPRESSION_NONE = 0,	/**< No compression */
	PIF_COMPRESSION_RLE			/**< RLE compression */
}pifCompression;

/** Operation state in indexed mode*/
typedef enum {
	PIF_INDEXED_NORMAL_OPERATION = 0,	/**< Normal operation */
	PIF_INDEXED_BYPASS_LOOKUP = 1		/**< Ignore index table, send index value to user */
}pifIndexedBypass;

/** PIF file structure and image information */
typedef struct {
	uint32_t fileSize;				/**< File Size of the Image */
	uint32_t imageOffset;			/**< Index Offset to the image data array */
	pifImageType imageType:4;		/**< Image Type */
	uint16_t bitsPerPixel;			/**< BitsPerPixel */
	uint16_t imageWidth;			/**< Image Width in Pixel */
	uint16_t imageHeight;			/**< Image Height in Pixel */
	uint32_t imageSize;				/**< Image Data Size in Bytes */
	uint16_t colTableSize;			/**< Color Table Size in Bytes */
	pifCompression compression:4;	/**< RLE Compression enabled or not */
	uint16_t startX;				/**< Display Start Positon X */
	uint16_t startY;				/**< Display Start Position Y */
	uint16_t currentX;				/**< Current X Positon of the image being processed */
	uint16_t currentY;				/**< Current Y Position of the image being processed */
}pifINFO_t;

// Callbacks are expected to return 0. Non-zero return is treatet as an error!
/** 
 * @brief Drawing callbacks: Prepare display
 * 
 * Readies the display for an image, that is about to be displayed.
 * All required informations (size, color format & more) are within
 * p_pifInfo.
 * @param p_Display		Generic void pointer holding possible display identifiers
 * @param p_pifInfo		pifINFO_t pointer, containing information about the current image
 * @return Expects non-NULL if an error is encountered
 */
typedef int8_t (PIF_PREPARE_IMAGE)(void *p_Display, pifINFO_t* p_pifInfo);

/** 
 * @brief Drawing callbacks: Drawing pixel per pixel on the display 
 * 
 * Drawing the pixel from the image on the display. Ensure that the right
 * pixel format is interpreted by checking the pifINFO_t pointer -> imageType!
 * @param p_Display		Generic void pointer holding possible display identifiers
 * @param p_pifInfo		pifINFO_t pointer, containing information about the current image
 * @param pixel			Color data of the current pixel to be drawn
 */
typedef void (PIF_DRAW_PIXEL)(void *p_Display, pifINFO_t* p_pifInfo, uint32_t pixel);

/** 
 * @brief Drawing callbacks: Finishing the drawing operation 
 * 
 * Signals the display that the image has been displayed. Can be used to push out
 * a possibly buffered image to the actual display.
 * @param p_Display		Generic void pointer holding possible display identifiers
 * @param p_pifInfo		pifINFO_t pointer, containing information about the current image
 * @return Expects non-NULL if an error is encountered
 */
typedef int8_t (PIF_FINISH_IMAGE)(void *p_Display, pifINFO_t* p_pifInfo);

/** @brief Painting structure
 * 
 * Contains drawing function pointers, display information and optional color table buffers */
typedef struct {
	PIF_PREPARE_IMAGE *prepare;	/**< Optional Function to allow the display operation to be prepared */
	PIF_DRAW_PIXEL *draw;		/**< Function to draw the pixel */
	PIF_FINISH_IMAGE *finish;	/**< Optional Function to finish the display operation */
	void *displayHandle;		/**< Display handler */
	pifIndexedBypass bypassColTable;	/**< Bypass the lookup of indexed colors, 
								handy for displays supporting very specific formats (like 7-colors e-ink displays) */
	uint8_t *colTableBuf;		/**< Optional array to buffer the color table */
	uint16_t colTableBufLen;	/**< Length of the color table buffer */
}pifPAINT_t;

// - FILE I/O FUNCTIONS REQUIRED FOR BASIC FUNCTIONALITY -
/**
 * @brief File I/O callback: Open file
 * 
 * I/O callbacks: Calls with pointer to string and pointer to char.
 * Expects a fileHandler pointer or a non-NULL value in *fileError 
 * @param pc_filepath	String path to the file to open
 * @param fileError		int8_t pointer, should be set non-NULL if an error occurred
 * @returns	Void pointer to the opened file
 */
typedef void* (PIF_OPEN_FILE)(const char* pc_filePath, int8_t *fileError);

/**
 * @brief File I/O callback: Close file
 * 
 * Closes the opened file
 * @param p_fileHandle	Void pointer to the open file
 * @returns	Non-NULL if an error occurred
 */
typedef int8_t (PIF_CLOSE_FILE)(void *p_fileHandle);

/**
 * @brief File I/O callback: Read data from file
 * 
 * Read data from the file into a buffer
 * @param p_fileHandle	Void pointer to the open file
 * @param p8_buf		Pointer to the Uint8 buffer for the data
 * @param legnth		Amount of bytes to read into the buffer
 */
typedef void (PIF_READ_FILE)(void *p_fileHandle, uint8_t *p8_buf, uint8_t length);

/**
 * @brief File I/O callback: Seek file position
 * 
 * Changes the file pointer to read at different positions
 * @param p_fileHandle	Void pointer to the open file
 * @param u32_filePos	Position to seek at
 * @return Non-NULL if an error occurred
*/
typedef int8_t (PIF_SEEK_FILE)(void *p_fileHandle, uint32_t u32_filePos);

/** File I/O structure */
typedef struct {
	PIF_OPEN_FILE *open;		/**< Required function pointer to open file */
	PIF_CLOSE_FILE *close;		/**< Optional function pointer to close the file */
	PIF_READ_FILE *readByte;	/**< Required function pointer to read x amount of bytes */
	PIF_SEEK_FILE *seekPos;		/**< Required function pointer to seek / move file position */
	uint32_t filePos;			/**< File index position used internally */
	void *fileHandle;			/**< File Handler used by the FILE I/O functions */
}pifIO_t;

/** Final PIF Handler */
typedef struct {
	pifINFO_t pifInfo;			/**< Information about the (last) opened image */
	pifPAINT_t *pifDecoder;		/**< Decoder to access the Display */
	pifIO_t *pifFileHandler;	/**< File Read Functions */
}pifHANDLE_t;

/**
 * @brief Setup the \a pifPAINT_t structure
 * 
 * This is an optional function to set up the \a pifPAINT_t structure. 
 * @param p_painter 			Pointer to a \a pifPAINT_t structure to set up
 * @param f_optional_prepare 	Optional pointer to a \a PIF_PREPARE_IMAGE function
 * @param f_draw 				Required pointer to a \a PIF_DRAW_PIXEL function
 * @param f_optional_finish 	Optional pointer to a \a PIF_FINISH_IMAGE function
 * @param p_displayHandler 		Void pointer to differentiate between displays - optional
 * @param p8_opt_ColTableBuf 	Pointer to an optional UINT8 buffer, to buffer the color table
 * @param u16_colTableBufLength Size of the color table buffer in bytes - set to zero if not used
 * @return Returns \a pifRESULT ;PIF_RESULT_DRAWERR when no f_draw pointer has been supplied, otherwise PIF_RESULT_OK
 */
pifRESULT pif_createPainter(pifPAINT_t *p_painter, PIF_PREPARE_IMAGE *f_optional_prepare, PIF_DRAW_PIXEL *f_draw,
		PIF_FINISH_IMAGE *f_optional_finish, void *p_displayHandler, uint8_t *p8_opt_ColTableBuf,
		uint16_t u16_colTableBufLength);

/**
 * @brief Setup the \a pifIO_t structure
 * 
 * This is an optional function to set up the \a pifIO_t structure.
 * Checks if the nessesary four function pointers are not NULL.
 * @param p_fileIO 		Pointer to a \a pifIO_t structure to be set up
 * @param f_openFile 	Required pointer to a \a PIF_OPEN_FILe function
 * @param f_closeFile 	Optional pointer to a \a PIF_CLOSE_FILe function
 * @param f_readFile 	Required pointer to a \a PIF_READ_FILe function
 * @param f_seekFile 	Required pointer to a \a PIF_SEEK_FILE	function
 * @return Returns \a pifRESULT ;PIF_RESULT_IOERR if any function pointer is NULL, otherwise PIF_RESULT_OK
 */
pifRESULT pif_createIO(pifIO_t *p_fileIO, PIF_OPEN_FILE *f_openFile, PIF_CLOSE_FILE *f_closeFile,
		PIF_READ_FILE *f_readFile, PIF_SEEK_FILE *f_seekFile);

/**
 * @brief Setup the \a pifHANDLE_t structure
 * 
 * This optional function sets up the \a pifHANDLE_t structure by linking
 * the \a pifIO_t and \a pifPAINT_t structures to it. No checks performed. 
 * @param p_PIF 		Pointer to a \a pifHANDLE_t structure
 * @param p_fileIO		Pointer to a \a pifIO_t struture
 * @param p_painter		Pointer to a \a pifPAINT_t structure
 */
void pif_createPIFHandle(pifHANDLE_t *p_PIF, pifIO_t *p_fileIO, pifPAINT_t *p_painter);

/**
 * @brief Open & parse PIF file
 * 
 * Attempts to open the file and parse the file header, storing the read image information
 * inside the pifInfo within the \a pifHANDLE_t structure.
 * Returns an error if the file couldn't be opened or if the image header isn't as expected. 
 * @param p_PIF 		Pointer to a initialized \a pifHANDLE_t structure
 * @param pc_path 		String-path to the PIF file to open
 * @return Returns \a pifRESULT to state if the operation was successful, or what kind of error encountered
 */
pifRESULT pif_open(pifHANDLE_t *p_PIF, const char *pc_path);

/**
 * @brief Display the PIF file
 * 
 * Reads the image data and decompresses or looks up the colors, if needed.
 * Sends the image data to show pixel by pixel to the display.
 * @param p_PIF 		Pointer to a initialized \a pifHANDLE_t structure
 * @param x0 			Start x position of the image on the screen
 * @param y0 			Start y position of the image on the screen
 * @return Returns \a pifRESULT to state if the operation was successful, or what kind of error encountered 
 */
pifRESULT pif_display(pifHANDLE_t *p_PIF, uint16_t x0, uint16_t y0);

/**
 * @brief Get PIF image information
 * 
 * Points the given \a pifINFO_t pointer to a the internal \a pifINFO_t structure,
 * which contains all the decoded information of the image (except the raw image data).
 * @param p_PIF 		Pointer to a initialized \a pifHANDLE_t structure
 * @param p_Info 		\a pifINFO_t pointer, will be overwritten within the function
 * @return Returns \a pifRESULT to state if the operation was successful, or what kind of error encountered  
 */
pifRESULT pif_getInfo(pifHANDLE_t *p_PIF, pifINFO_t *p_Info);

/**
 * @brief 
 * 
 * @param p_PIF 
 * @return Returns \a pifRESULT to state if the operation was successful, or what kind of error encountered   
 */
pifRESULT pif_close(pifHANDLE_t *p_PIF);

/**
 * @brief Load, display and close the PIF image
 * 
 * This function basically bundles \a pif_open, \a pif_display and \a pif_close, 
 * executing these functions in a row. If an error is encountered in any of these 
 * functions, further execution of the function is canceled and the error is returned.
 * @param p_PIF 		Pointer to a initialized \a pifHANDLE_t structure
 * @param pc_path		String-path to the PIF file to open
 * @param x0 			Start x position of the image on the screen
 * @param y0 			Start y position of the image on the screen
 * @return Returns \a pifRESULT to state if the operation was successful, or what kind of error encountered   
 */
pifRESULT pif_OpenAndDisplay(pifHANDLE_t *p_PIF, const char *pc_path, uint16_t x0, uint16_t y0);

/**
 * @brief Convert pixel image data
 * 
 * Convert the pixel image data from one type to the other type as accurate as possible
 * If possible, use images supported natively by the display - this is a helper function
 * to provide image support for any display type. Setting convMode to 1 enables a faster
 * conversion method at the cost of color accuracy.
 * @param color 		The pixel to convert
 * @param sourceType 	The \a pifImageType source type of the pixel
 * @param targetType 	The \a pifImageType target to convert to
 * @param convMode 		Selects which color conversion should be applied
 * @return Returns the converted pixel 
 */
uint32_t convertColor(uint32_t color, pifImageType sourceType, pifImageType targetType, pifColorConversion convMode);

#endif /* PIFDEC_H_ */