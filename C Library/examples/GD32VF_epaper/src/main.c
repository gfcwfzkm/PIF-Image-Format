#include "main.h"

int main(void)
{
	SPI_Master_t epaper_SPI;	// SPI Interface
	spd1656_t epd;				// E-Paper Handle
	pifIO_t pifIO;				// PIF I/O
	pifDEC_t pifDEC;			// PIF Draw
	pifHANDLE_t pifHandle;		// PIF Handle
	
	/* Initialise the GD32VF103 System */
	initMCU();

	/* Prepare SPI Interface, but with lower SPI speed */
	hwSPI_SetupInterface(&epaper_SPI, SPI1, GPIOB, DISPLAY_CS);
	epaper_SPI.clkdiv = HWSPI_CLKDIV_32;

	/* Prepare the E-Paper Interface */
	spd1656_create(&epd, &epaper_SPI, &hwSPI_InterfacePrepare, &hwSPI_InterfaceSendBytes, &hwSPI_InterfaceFinish, &getMS, &epaper_setPin, &epaper_checkBusy);
	
	/* Define the drawing functions for the PIF library */
	pif_createDecoder(&pifDEC, &prepStuff, &drawStuff, &finishStuff, &epd, 0, 0);
	/* Set optional argument to bypass the lookup table, directly sending the indexing-value to the drawing function */
	pifDEC.bypassColTable = PIF_INDEXED_BYPASS_LOOKUP;

	/* Define the I/O functions to read the included array */
	pif_createIO(&pifIO, &openStuff, &closeStuff, &readStuff, &seekStuff);

	/* Define the PIF handle */
	pif_createPIFHandle(&pifHandle, &pifIO, &pifDEC);

	/* Initialise the 4.01" 7-color e-paper display */
	spd1656_init(&epd);
	/* Display the 7-color test pattern */
	spd1656_TestPattern(&epd);
	delay_ms(2000);

	/* Display the PIF file */
	pif_OpenAndDisplay(
		&pifHandle,	// PIF handle
		"0",		// PIF file name, in this case not important as only a specific array is read
		0,			// X-offset on the display, not used on the e-paper
		0			// Y-offset on the display, not used on the e-paper
	);

    while (1) {
		/* Nothing- */
		delay_ms(500);
		GPIO_OCTL(GPIOC) ^= ONBOARDLED;
    }
}

void initMCU(void)
{
	rcu_periph_clock_enable(RCU_GPIOC);
	rcu_periph_clock_enable(RCU_GPIOB);
	rcu_periph_clock_enable(RCU_SPI1);

	gpio_init(GPIOC, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, ONBOARDLED);
	gpio_init(GPIOB, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, SPI_MOSI | SPI_SCK);
	gpio_init(GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, DISPLAY_CS | DISPLAY_DC | DISPLAY_RST);
	gpio_init(GPIOB, GPIO_MODE_IPD, GPIO_OSPEED_50MHZ, SPI_MISO);
	gpio_init(GPIOB, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, DISPLAY_BSY);

	gpio_bit_set(GPIOB, DISPLAY_CS);
}

uint32_t iseekPos = 0;
int8_t prepStuff(void *p_Display, pifINFO_t* p_pifInfo)
{
	if ((p_pifInfo->imageType < PIF_TYPE_IND8) || (p_pifInfo->colTableSize != 7))
	{
		// We only want to support indexed images here
		// that fit the right format for our display
		return 1;
	}
	// Prepare the display for drawing
	spd1656_StartUpdate(p_Display);
	return 0;
}

void drawStuff(void *p_Display, pifINFO_t* p_pifInfo, uint32_t pixel)
{
	// Forward the indexed image data directly to the display
	spd1656_writePixel(p_Display, (uint8_t)pixel);
}

int8_t finishStuff(void *p_Display, pifINFO_t* p_pifInfo)
{
	// Finish the drawing process by initiating the e-ink refresh
	spd1656_FinishUpdate(p_Display);
	return 0;
}

void* openStuff(const char* pc_filePath, int8_t *fileError)
{
	// Since we are reading from flash, not much to prepare here
	*fileError = 0;
	iseekPos = 0;
	return 0;
}

int8_t closeStuff(void *p_file)
{
	// Nothing to close
	return 0;
}

void readStuff(void *p_file, uint8_t *p_buffer, uint8_t size)
{
	// Simply read the amount of requested bytes from the byte array
	for (uint8_t i = 0; i < size; i++)
	{
		// Read the next byte from the array and increase the seeking pointer
		p_buffer[i] = redpaz[iseekPos++];
	}
}

int8_t seekStuff(void *p_file, uint32_t offset)
{
	// Change the seeking position
	iseekPos = offset;
	return 0;
}

void epaper_setPin(void* epd, spd1656_pin epd_pin, uint8_t ioState)
{
	switch (epd_pin)
	{
	case SPD1656_RESET_PIN:
		if (ioState)
		{
			gpio_bit_set(GPIOB, DISPLAY_RST);
		}
		else
		{
			gpio_bit_reset(GPIOB, DISPLAY_RST);
		}
		break;
	case SPD1656_DC_PIN:
		if (ioState)
		{
			gpio_bit_set(GPIOB, DISPLAY_DC);
		}
		else
		{
			gpio_bit_reset(GPIOB, DISPLAY_DC);
		}
		break;
	}
}

uint8_t epaper_checkBusy(void* epd)
{
	if (gpio_input_bit_get(GPIOB, DISPLAY_BSY))
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

void delay_ms(uint32_t count)
{
	uint64_t start_mtime, delta_mtime;

	// Don't start measuruing until we see an mtime tick
	uint64_t tmp = get_timer_value();
	do {
	start_mtime = get_timer_value();
	} while (start_mtime == tmp);

	do {
	delta_mtime = get_timer_value() - start_mtime;
	}while(delta_mtime <(SystemCoreClock/4000.0 *count ));
}


void delay_us(uint32_t count)
{
	uint64_t start_mtime, delta_mtime;

	// Don't start measuruing until we see an mtime tick
	uint64_t tmp = get_timer_value();
	do {
	start_mtime = get_timer_value();
	} while (start_mtime == tmp);

	do {
	delta_mtime = get_timer_value() - start_mtime;
	}while(delta_mtime <(SystemCoreClock/4000000.0 *count ));
}


uint32_t getMS(void)
{
    return (uint32_t)(get_timer_value() / (SystemCoreClock / 4000));
}