#include "spd1656.h"

void spd1656_command(spd1656_t *epd, uint8_t cmd)
{
	epd->controlIO(&epd, SPD1656_DC_PIN, 0);

	epd->startTransaction(epd->ioInterface);
	epd->sendBytes(epd->ioInterface, 0, &cmd, 1);
	epd->endTransaction(epd->ioInterface);
}

void spd1656_data(spd1656_t *epd, uint8_t data)
{
	epd->controlIO(&epd, SPD1656_DC_PIN, 1);

	epd->startTransaction(epd->ioInterface);
	epd->sendBytes(epd->ioInterface, 0, &data, 1);
	epd->endTransaction(epd->ioInterface);
}

uint8_t spd1656_busyWait(spd1656_t *epd, uint32_t msTimeout)
{
	uint32_t tickStart = epd->getMillis();

	while(epd->checkBusy(epd))	
	{
		if ((epd->getMillis() - tickStart) >= msTimeout)
		{
			return 1;
		}
	}
	return 0;
}

void spd1656_create(spd1656_t *epd, void *ioH, uint8_t (*ioStart)(void*),
					uint8_t (*ioSend)(void*, uint8_t, uint8_t*, uint16_t),
					uint8_t (*ioEnd)(void*), uint32_t (*getMillis)(void),
					void (*controlIO)(void*, spd1656_pin, uint8_t),
					uint8_t (*checkBusy)(void*))
{
	epd->ioInterface = ioH;
	epd->startTransaction = ioStart;
	epd->sendBytes = ioSend;
	epd->endTransaction = ioEnd;
	epd->getMillis = getMillis;
	epd->controlIO = controlIO;
	epd->checkBusy = checkBusy;
}

uint8_t spd1656_init(spd1656_t *epd)
{
	uint32_t resetWait = epd->getMillis();	
	epd->controlIO(&epd, SPD1656_RESET_PIN, 1);
	while((epd->getMillis() - resetWait) < 200)	{}
	epd->controlIO(&epd, SPD1656_RESET_PIN, 0);
	resetWait = epd->getMillis();	
	while((epd->getMillis() - resetWait) < 1)	{}
	epd->controlIO(&epd, SPD1656_RESET_PIN, 1);
	resetWait = epd->getMillis();	
	while((epd->getMillis() - resetWait) < 200)	{}
	if (spd1656_busyWait(epd, 100))	return 1;

	spd1656_command(epd, REG_SPD1656_PSR);
	spd1656_data(epd, 0x2f);
	spd1656_data(epd, 0x00);
	spd1656_command(epd, REG_SPD1656_PWR);
	spd1656_data(epd, 0x37);
	spd1656_data(epd, 0x00);
	spd1656_data(epd, 0x05);
	spd1656_data(epd, 0x05);
	spd1656_command(epd, REG_SPD1656_PFS);
	spd1656_data(epd, 0x00);
	spd1656_command(epd, REG_SPD1656_BTST);
	spd1656_data(epd, 0xC7);
	spd1656_data(epd, 0xC7);
	spd1656_data(epd, 0x1D);
	spd1656_command(epd, REG_SPD1656_TSE);
	spd1656_data(epd, 0x00);
	spd1656_command(epd, REG_SPD1656_CDI);
	spd1656_data(epd, 0x37);
	spd1656_command(epd, REG_SPD1656_TCON);
	spd1656_data(epd, 0x22);
	spd1656_command(epd, REG_SPD1656_TRES);
	spd1656_data(epd, 0x02);
	spd1656_data(epd, 0x80);
	spd1656_data(epd, 0x01);
	spd1656_data(epd, 0x90);
	spd1656_command(epd, REG_SPD1656_PWS);
	spd1656_data(epd, 0xAA);
	
	epd->updateNeeded = 0;
	epd->pixelCache = 0;
	epd->pixelInCache = 0;

	return 0;
}

// Starts update sequence, must be called before sending pixels to the display
void spd1656_StartUpdate(spd1656_t* epd)
{
	epd->updateNeeded = 0;

	//Resolution Setting
	spd1656_command(epd, REG_SPD1656_TRES);
	spd1656_data(epd, 0x02);
	spd1656_data(epd, 0x80);
	spd1656_data(epd, 0x01);
	spd1656_data(epd, 0x90);

	//Data transmission start DTM1
	spd1656_command(epd, REG_SPD1656_DTM1);
}

// Send Pixels to the display
// Called after spd1656_update!
void spd1656_writePixel(spd1656_t* epd, spd1656_color color)
{
	if (epd->pixelInCache)
	{
		spd1656_data(epd, epd->pixelCache << 4 | color);
		epd->pixelInCache = 0;
	}
	else
	{
		epd->pixelCache = color;
		epd->pixelInCache = 1;
	}
	
	spd1656_busyWait(epd, 60000);
}

uint8_t spd1656_FinishUpdate(spd1656_t* epd)
{
	uint8_t timeoutResult = 0;

	// Last pixel in buffer? dump it
	if (epd->pixelInCache)
	{
		spd1656_data(epd, epd->pixelCache << 4 | 0);
		epd->pixelInCache = 0;
	}

	// Power ON
	spd1656_command(epd, REG_SPD1656_PON);
	timeoutResult |= spd1656_busyWait(epd, 100);

	// Display refresh
	spd1656_command(epd, REG_SPD1656_DRF);
	timeoutResult |= spd1656_busyWait(epd, 60000);

	// Power off
	spd1656_command(epd, REG_SPD1656_POF);
	timeoutResult |= spd1656_busyWait(epd, 100);

	return timeoutResult;
}

uint8_t spd1656_clear(spd1656_t* epd, spd1656_color color)
{
	spd1656_StartUpdate(epd);

	for (uint16_t i = 0; i < SPD1656_DISPLAY_HEIGHT * SPD1656_DISPLAY_WIDTH; i++)
	{
		spd1656_writePixel(epd, color);
	}

	return spd1656_FinishUpdate(epd);
}

uint8_t spd1656_TestPattern(spd1656_t* epd)
{
	spd1656_StartUpdate(epd);

	for (uint16_t i = 0; i < SPD1656_DISPLAY_HEIGHT; i++)
	{
		for (spd1656_color k = 0; k < 7; k++)
		{
			for (uint16_t j = 0; j < 91; j++)
			{
					spd1656_writePixel(epd, k);
			}
		}
		spd1656_writePixel(epd, SPD1656_COLOR_ORANGE);
		spd1656_writePixel(epd, SPD1656_COLOR_ORANGE);
		spd1656_writePixel(epd, SPD1656_COLOR_ORANGE);
	}

	return spd1656_FinishUpdate(epd);
}