#ifndef _SPD1656_H_
#define _SPD1656_H_

#include <inttypes.h>

#define SPD1656_DISPLAY_HEIGHT	400
#define SPD1656_DISPLAY_WIDTH	640

#define REG_SPD1656_PSR		0x00
#define REG_SPD1656_PWR		0x01
#define REG_SPD1656_POF		0x02
#define REG_SPD1656_PFS		0x03
#define REG_SPD1656_PON		0x04
#define REG_SPD1656_BTST	0x06
#define REG_SPD1656_DSLP	0x07
#define REG_SPD1656_DTM1	0x10
#define REG_SPD1656_DSP		0x11
#define REG_SPD1656_DRF		0x12
#define REG_SPD1656_WINM	0x14
#define REG_SPD1656_WHRES	0x15
#define REG_SPD1656_WVRES	0x16
#define REG_SPD1656_LUTC	0x20
#define REG_SPD1656_LUTB	0x21
#define REG_SPD1656_LUTW	0x22
#define REG_SPD1656_LUTG1	0x23
#define REG_SPD1656_LUTG2	0x24
#define REG_SPD1656_LUTR0	0x25
#define REG_SPD1656_LUTR1	0x26
#define REG_SPD1656_LUTR2	0x27
#define REG_SPD1656_LUTR3	0x28
#define REG_SPD1656_LUTXON	0x29
#define REG_SPD1656_PLL		0x30
#define REG_SPD1656_TSC		0x40
#define REG_SPD1656_TSE		0x41
#define REG_SPD1656_TSW		0x42
#define REG_SPD1656_TSR		0x43
#define REG_SPD1656_CDI		0x50
#define REG_SPD1656_LPD		0x51
#define REG_SPD1656_TCON	0x60
#define REG_SPD1656_TRES	0x61
#define REG_SPD1656_DAM		0x65
#define REG_SPD1656_FLG		0x71
#define REG_SPD1656_AMV		0x80
#define REG_SPD1656_VV		0x81
#define REG_SPD1656_VDCS	0x82
#define REG_SPD1656_CCSET	0xE0
#define REG_SPD1656_PWS		0xE3

typedef enum {
	SPD1656_COLOR_BLACK = 0,
	SPD1656_COLOR_WHITE = 1,
	SPD1656_COLOR_GREEN = 2,
	SPD1656_COLOR_BLUE  = 3,
	SPD1656_COLOR_RED   = 4,
	SPD1656_COLOR_YELLOW = 5,
	SPD1656_COLOR_ORANGE = 6
} spd1656_color;

typedef enum {
	SPD1656_RESET_PIN,
	SPD1656_DC_PIN,
} spd1656_pin;

typedef struct
{
	uint8_t updateNeeded:1;
	uint8_t pixelInCache:1;
	uint8_t pixelCache;
	void *ioInterface;					// Pointer to the IO/Peripheral Interface library
	// Any return value by the IO interface functions have to return zero when successful or
	// non-zero when not successful.
	uint8_t (*startTransaction)(void*);	// Prepare the IO/Peripheral Interface for a transaction
	uint8_t (*sendBytes)(void*,			// Send data function pointer: InterfacePointer,
						uint8_t,		// Address of the PortExpander (8-Bit Address Format!),
						uint8_t*,		// Pointer to send buffer,
						uint16_t);		// Amount of bytes to send
	uint8_t (*endTransaction)(void*);	// Finish the transaction / Release IO/Peripheral
	uint32_t (*getMillis)(void);		// Get the current time in milliseconds
	void (*controlIO)(void*,
					spd1656_pin,
					uint8_t);
	uint8_t (*checkBusy)(void*);
} spd1656_t;

void spd1656_create(spd1656_t *epd, void *ioH, uint8_t (*ioStart)(void*),
					uint8_t (*ioSend)(void*, uint8_t, uint8_t*, uint16_t),
					uint8_t (*ioEnd)(void*), uint32_t (*getMillis)(void),
					void (*controlIO)(void*, spd1656_pin, uint8_t),
					uint8_t (*checkBusy)(void*));

uint8_t spd1656_init(spd1656_t *epd);

uint8_t spd1656_busyWait(spd1656_t *epd, uint32_t msTimeout);

void spd1656_command(spd1656_t *epd, uint8_t cmd);

void spd1656_data(spd1656_t *epd, uint8_t data);

void spd1656_StartUpdate(spd1656_t* epd);

void spd1656_writePixel(spd1656_t* epd, spd1656_color color);

uint8_t spd1656_FinishUpdate(spd1656_t* epd);

uint8_t spd1656_clear(spd1656_t* epd, spd1656_color color);

uint8_t spd1656_TestPattern(spd1656_t* epd);

#endif