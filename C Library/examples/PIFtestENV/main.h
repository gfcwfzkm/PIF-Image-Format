#ifndef MAIN_H_
#define MAIN_H_

#include <avr/io.h>
#include <avr/interrupt.h>
#include <time.h>
#include "io_map.h"
#include "lowlevel/gesiFramework.h"			// millis tick function
#include "lowlevel/hwSPI.h"					// SPI Driver
#include "lowlevel/twi_master_driver.h"		// I2C Driver
#include "lowlevel/xmegaUSART/uart.h"		// UART Driver
#include "lowlevel/xmegaUARTSPI/uartSPI.h"	// SPI Driver using UART Peripheral
#include "icdriver/mcp23.h"					// MCP23X08 / MCP23X17 Port Expander Driver
#include "icdriver/spiflash/spiflash.h"		// W25Q512 (64MByte) Memory Chip Driver
#include "icdriver/RTC-RV3028/rtc_rv3028.h"	// RV3028 I2C RTC Driver
#include "icdriver/RA8876/RA8876.h"			// RA8876 Display Driver
#include "icdriver/RA8876/touch.h"			// Capacitive Touch Sensor Driver
#include "icdriver/RA8876/RA8876_cursors.h"	// Extra generic cursors
#include "midlevel/ff.h"					// FATFS
#include "midlevel\diskio.h"				// FATFS Disk IO
#include "midlevel/flashcache.h"			// SPIFlash <-> FATFS Glue Driver
#include "midlevel/gshell/gshell.h"			// Interactive Serial Console
#include "midlevel/file_modem/file_modem.h"	// X-Modem to load files onto SPIFlash
#include "midlevel/RA8876Helper.h"			// Helper functions, .BMP function for Display
#include "midlevel/pifdec/pifdec.h"			// PIF Image Decoder

#define STATLED_BUSY()				STATPORT.OUTCLR = STATLED
#define STATLED_FREE()				STATPORT.OUTSET = STATLED
#define STATLED_TGL()				STATPORT.OUTTGL = STATLED

/* Console Shell Setting */
#define SHELL_ARGUMENT_LENGTH		20

/* Shell Delay Setting */
#define GFD_COMMANDINTERFACE		50


#define UART_BUFFER_SIZE	128		// Both RX and TX FIFO Buffer
#define UART_BAUD_RATE		460800	// Max Speed Supported by MCP2221A USB to Serial Driver
#define TWI_SPEED			400000

typedef struct {
	USARTSPI_Master_t uSPI;
	TWI_Master_t twiMaster;
	BUF_UART_t uartConsole;
	RA8876_SPI_t dispSPI;
	
	mcp23x_t ioExp;
	touch_t dispTouch;
	flash_t mainFlash;
	flash_t dispFlash;
	flashcache_t flashBuffer;
	FATFS fs;
	RV3028_t rtcClk;
} microcontrollerBoard_t;

/* Initialise Microcontroller Board */
void mcuboard_init(void);
/* Handle / Process UART Shell */
void mcu_processShell(void);

#endif /* MAIN_H_ */