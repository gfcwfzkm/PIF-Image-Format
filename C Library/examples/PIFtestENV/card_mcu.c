/*
 * card_mcu.c
 *
 * Created: 22.06.2021 10:12:28
 */ 

#include "main.h"

extern microcontrollerBoard_t *mcuBoard;
register8_t newRTC_tick = 0;

ISR(USARTE0_RXC_vect)
{
	uart_receive_interrupt(&(mcuBoard->uartConsole));
}

ISR(USARTE0_DRE_vect)
{
	uart_transmit_interrupt(&(mcuBoard->uartConsole));
}

ISR(TWIE_TWIM_vect)
{
	TWI_MasterInterruptHandler(&(mcuBoard->twiMaster));
}

ISR(PORTE_INT0_vect)
{
	newRTC_tick = 1;
	system_tick();
}

/* Notwendige Buffer für diverse Bibliotheken */
static uint8_t receiveBuffer[UART_BUFFER_SIZE];
static uint8_t transmitBuffer[UART_BUFFER_SIZE];

static void _mcu_init()
{
	/* CPU-Takt auf 32 MHz einstellen */
	OSC.CTRL |= OSC_RC32MEN_bm | OSC_RC2MEN_bm | OSC_RC32KEN_bm;// 32MHz Oszillator aktivieren
	while(!(OSC.STATUS & OSC_RC32MRDY_bm));				// Warten bis der 32MHz Oszillator stabil ist
	while(!(OSC.STATUS & OSC_RC2MRDY_bm));				// Warten bis der 2MHz Oszillator stabil ist
	while(!(OSC.STATUS & OSC_RC32KRDY_bm));				// Warten bis der (interner) 32kHz Oszillator stabil ist
	
	/* Kalibrierung des 32MHz Taktes mit Hilfe des internen 32kHz Oszillators */
	// OSC.DFLLCTRL = 0;
	DFLLRC32M.CTRL = DFLL_ENABLE_bm;
	
	_PROTECTED_WRITE(CLK.CTRL, CLK_SCLKSEL_RC32M_gc);	// 32MHz als Haupttakt wählen
	
	OSC.CTRL &=~ OSC_RC2MEN_bm;		// 2MHz Oszillator deaktivieren, da nicht mehr notwendig
	
	/* IO Setup */
	PORTCFG.MPCMASK = PIN7_bm | PIN6_bm | PIN5_bm | PIN4_bm | PIN3_bm | PIN2_bm | PIN1_bm | PIN0_bm;
	DATAPORT.PIN0CTRL = PORT_OPC_PULLDOWN_gc;
	
	ADDRPORT.DIRSET = ADDR_MCS | ADDR_BD3 | ADDR_BD2 | ADDR_BD1 | ADDR_BD0 | ADDR_IC2 | ADDR_IC1 | ADDR_IC0;
	
	SERPORT.DIRSET = SPI_SCK | SPI_MOSI | DB_RS | UART_TX |UART_XCK | DB_EN;
	SERPORT.PIN2CTRL = PORT_OPC_PULLDOWN_gc;	// UART_RX
	SERPORT.PIN6CTRL = PORT_OPC_PULLDOWN_gc;	// SPI_MISO
	
	MIXPORT.DIRSET = LCD_CS | MEM_CS | IMOSI | ISCK | DB_RW;
	MIXPORT.PIN2CTRL = PORT_OPC_PULLDOWN_gc;	// IMISO

	MISCPORT.DIRSET = DBG_TX | RESET;	
	PORTCFG.MPCMASK = SDA | SCL;
	MISCPORT.PIN0CTRL = PORT_OPC_PULLUP_gc;
	PORTCFG.MPCMASK = DBG_RX | TOUCH_IRQ | PE_IRQ | RTC_SIG;
	MISCPORT.PIN7CTRL = PORT_OPC_PULLDOWN_gc;
	
	LCDPORT.DIRSET = LCD_RST;
	PORTCFG.MPCMASK = RE_SW | RE_A | RE_B;
	LCDPORT.PIN3CTRL = PORT_OPC_PULLDOWN_gc;
	PORTCFG.MPCMASK = LCD_WAIT | LCD_IRQ | IRQH | IRQM;
	LCDPORT.PIN1CTRL = PORT_OPC_PULLUP_gc;
	
	STATPORT.DIRSET = STATLED;
	STATPORT.PIN0CTRL = PORT_OPC_PULLUP_gc;
	
	/* Enable LO interrupt level. */
	PMIC.CTRL |= PMIC_LOLVLEN_bm;
	sei();
}

static void _dispInit()
{
	RA8876_hwReset();
	RA8876_init();
	RA8876_brightness(1);
	RA8876_clearMemory(0xFFFF);
	RA8876_displayOnOff(1);
	
	RA8876_graphicCursorSelect(0);
	RA8876_graphicCursorLoad_f(RA8876_gImage_pen_il);
	RA8876_graphicCursorSelect(1);
	RA8876_graphicCursorLoad_f(RA8876_gImage_arrow_il);
	RA8876_graphicCursorSelect(2);
	RA8876_graphicCursorLoad_f(RA8876_gImage_busy_im);
	RA8876_graphicCursorSelect(3);
	RA8876_graphicCursorLoad_f(RA8876_gImage_no_im);
	RA8876_graphicCursorColor(0xFF, 0x00);
	
	RA8876_CanvasStartAddr(LAYER_0);
	RA8876_MainWindowStartAddress(LAYER_0);
	
	RA8876_graphicCursorSelect(2);
	RA8876_graphicCursorCoords(496, 550);
	RA8876_graphicCursorEnable();
	RA8876_FontInternalCGROM(ISO8859_1);
	
	RA8876_setTextSize(TEXTSIZE_12x24_24x24);
	RA8876_setTextCoords(0, 0);
	
	RA8876_spi_selectFlash(1);
	RA8876_spi_FlashAddressMode(FLASH_32_BIT_ADDRESS);
	RA8876_spi_mode(SPI_MODE_3);
	RA8876_spi_FlashDummyRead(NORMAL_ONE_DUMMY_READ);
	RA8876_spi_clockPeriod(8);
	RA8876_spi_enable();
}

static void _initTouch(touch_t *tdisp)
{
	gs_log_f(GLOG_INFO, "Initialising the touch controller...");
	if (tp_initChip(tdisp))
	{
		gs_log_f(GLOG_FATAL, "Error while initialising the touch controller!");
	}
	else
	{
		gs_log_f(GLOG_OK, "Touchdisplay-Kontroller initialized");
	}
}

static int _eu_dst(const time_t * timer, int32_t * z)
{
	uint32_t t = *timer;
	if ((uint8_t)(t >> 24) >= 194) t -= 3029443200U;
	t = (t + 655513200) / 604800 * 28;
	if ((uint16_t)(t % 1461) < 856) return 3600;
	else return 0;
}

static void _initRTC(RV3028_t *rtcClk)
{
	RV3028_TIME_t rtcTime;
	struct tm rtc_time;
	uint8_t tempVal;
	
	gs_log_f(GLOG_INFO, "Initialising the Real-Time-Clock...");
	if (rv3028_init(rtcClk))
	{
		rtcTime.seconds = 0;
		rtcTime.minutes = 0;
		rtcTime.hours	= 0;
		rtcTime.date	= 1;
		rtcTime.month	= 1;
		rtcTime.year	= 22;
		gs_log_f(GLOG_ERROR, "Couldn't initialise the RTC!");
		gs_log_f(GLOG_WARN,"Setting the 1st January 2022 at Midnight as date!");
	}
	else
	{
		gs_log_f(GLOG_OK, "RTC initialized!");
		/* Run initial RTC Setup if needed */
		if (rv3028_readReg(rtcClk, RV3028_R_USER_RAM_1) != 0xA8)
		{
			rv3028_writeReg(rtcClk, RV3028_R_CONTROL_1, RV3028_R_CONTROL_1_EERD);
			rv3028_writeReg(rtcClk, RV3028_R_EEPROM_CLKOUT,
			RV3028_R_EEPROM_CLKOUT_CLKOE | RV3028_R_EEPROM_CLKOUT_CLKSY |
			RV3028_R_EEPROM_CLKOUT_FD2 | RV3028_R_EEPROM_CLKOUT_FD0);
			tempVal = rv3028_readReg(rtcClk, RV3028_R_EEPROM_BACKUP);
			tempVal &= RV3028_R_EEPROM_BACKUP_EEOFFST0;
			tempVal |= RV3028_R_EEPROM_BACKUP_FEDE | RV3028_R_EEPROM_BACKUP_BSM1 |
			RV3028_R_EEPROM_BACKUP_BSM0;
			rv3028_writeReg(rtcClk, RV3028_R_EEPROM_BACKUP, tempVal);
			rv3028_updateConfigEEPROM(rtcClk);
			rv3028_writeReg(rtcClk, RV3028_R_USER_RAM_1, 0xA8);
			gs_log_f(GLOG_INFO, "RTC Settings & Memory updated!");
		}
		
		rv3028_getTime(rtcClk, &rtcTime);
		gs_log_f(GLOG_INFO, "Read Time: %.2d:%.2d:%.2d %.2d.%.2d.%.4d",
		rtcTime.hours, rtcTime.minutes,	rtcTime.seconds, rtcTime.date,
		rtcTime.month, rtcTime.year + 2000);
		
		if (rtcTime.year < 22)
		{
			rtcTime.seconds = 0;
			rtcTime.minutes = 0;
			rtcTime.hours	= 0;
			rtcTime.date	= 1;
			rtcTime.month	= 1;
			rtcTime.year	= 22;
			rv3028_setTime(rtcClk, &rtcTime);
			rv3028_getTime(rtcClk, &rtcTime);
			gs_log_f(GLOG_WARN, "Backup-Battery of the RTC Empty?");
		}
	}
	
	MISCPORT.INT0MASK |= RTC_SIG;
	MISCPORT.INTCTRL |= PORT_INT0LVL_LO_gc;
	MISCPORT.PIN7CTRL |= PORT_ISC_RISING_gc;
	
	rtc_time.tm_sec		= rtcTime.seconds;
	rtc_time.tm_min		= rtcTime.minutes;
	rtc_time.tm_hour	= rtcTime.hours;
	rtc_time.tm_mday	= rtcTime.date;
	rtc_time.tm_wday	= rtcTime.weekday;
	rtc_time.tm_mon		= rtcTime.month - 1;
	rtc_time.tm_year	= rtcTime.year + 100;
	rtc_time.tm_isdst	= 0;
	
	set_dst(_eu_dst);
	set_zone(1 * ONE_HOUR);
	set_system_time( mktime( (struct tm*)&rtc_time ) );
}

static void _initInternalPortExpander(mcp23x_t *mcp)
{
	gs_log_f(GLOG_INFO, "Initialise On-Board Port-Expander...");
	if (mcp23x_initChip(mcp))
	{
		gs_log_f(GLOG_ERROR, "Initialising the Port-Expander failed!");
	}
	else
	{
		gs_log_f(GLOG_OK, "Port-Expander initialized!");
		mcp23x_writePort(mcp, MCP23_PORTA, MCU_PE_DEFAULT_OUT);
		mcp23x_dirPort(mcp, MCP23_PORTA, MCU_PE_DEFAULT_DDR);
		_delay_us(500);
		mcp23x_writePort(mcp, MCP23_PORTA, MCU_PE_USBHUB_RST);
		_delay_ms(100);
	}
}

DWORD get_fattime(void)
{
	time_t rawTime;
	time(&rawTime);
	struct tm *_time = localtime(&rawTime);
	uint32_t fattime = ((uint32_t)(_time->tm_year - 80) << 25) |
	((uint32_t)(_time->tm_mon + 1) << 21) |
	((uint32_t)_time->tm_mday << 16) |
	(_time->tm_hour << 11) |
	(_time->tm_min << 5) |
	(_time->tm_sec >> 1);
	return fattime;
}

static void _initMemFS(flash_t *spiflash, flashcache_t *cache, FATFS *fs)
{
	char textlblbuf[32];
	uint32_t tempu32;
	
	gs_log_f(GLOG_INFO, "Initialising the Flash Memory Chip...");
	if (flash_init(spiflash, (1UL << 26), 0xEF))
	{
		gs_log_f(GLOG_FATAL, "Error while initialising the memory chip!");
	}
	else
	{
		gs_log_f(GLOG_OK, "Memory Chip initialised!");
		fc_init(cache,spiflash);
		
		gs_log_f(GLOG_INFO, "Checking the file system...");
		FRESULT _initRes = f_mount(fs, "0:", 1);
		if (_initRes == FR_NO_FILESYSTEM)
		{
			f_unmount("0:");
			gs_log_f(GLOG_FATAL, "Dateisystem konnte nicht initialisiert werden!");
		}
		else if (_initRes != FR_OK)
		{
			if (_initRes != FR_OK)
			{
				gs_log_f(GLOG_FATAL, "Dateisystem konnte nicht initialisiert werden!");
			}
		}
		
		if (_initRes == FR_OK)
		{
			f_getlabel("0:", textlblbuf, 0);
			f_getfree("0:", &tempu32, &fs);
			gs_log_f(GLOG_OK, "Filesystem [%s] found and initialised:", textlblbuf);
			gs_log_f(GLOG_INFO, "Total Capacity: %5lu KiB ", ((fs->n_fatent - 2) * fs->csize) / 2);
			gs_log_f(GLOG_INFO, "Free Memory:    %5lu KiB",(tempu32 * fs->csize) / 2);
		}
		
		//f_unmount("0:");
		//cache_syncBlocks(0);
	}
}

static void gsh_putc(char c)
{
	uart_putc(&(mcuBoard->uartConsole), c);
}

static uint8_t fm_recByte(uint8_t *ch, uint16_t timeout_ms)
{
	gf_delay_t timeout = {gf_millis(), (uint32_t)timeout_ms};
	uint16_t recCh;
	
	while(!gf_TimeCheck(&timeout, gf_millis()))
	{
		recCh = uart_getc(&(mcuBoard->uartConsole));
		if ((recCh >> 8) == UART_DATA_AVAILABLE)
		{
			*ch = (uint8_t)recCh;
			return 0;
		}
	}
	return 1;
}

static void fm_sendByte(uint8_t ch)
{
	uart_putc(&(mcuBoard->uartConsole), ch);
}

static void fm_flushRx(void)
{
	do 
	{
		asm("nop");
	} while ((uart_getc(&(mcuBoard->uartConsole)) >> 8) != UART_NO_DATA);
}

FIL imageFile;
void *ImageOpen(const char *path, int8_t *result)
{
	FRESULT res;
	res = f_open(&imageFile, path, FA_READ);
	gs_log_f((res == FR_OK) ? GLOG_INFO : GLOG_ERROR, "[PIF] Opening Image '%s'... %s! Code %"PRIu8, path, (res == FR_OK) ? "OK" : "FAILED", res);
	if (res != FR_OK)
	{
		*result |= 1;
	}
	
	return &imageFile;
}

int8_t ImageClose(void *pHandle)
{
	FRESULT res = f_close(pHandle);
	gs_log_f(GLOG_INFO, "[PIF] Closing file... %s! Code %"PRIu8, (res == FR_OK) ? "OK" : "FAILED", res);
	if (res != FR_OK)
	{
		return 1;
	}
	return 0;
}

void ImageReadB(void *pHandle, uint8_t *data, uint8_t length)
{
	UINT recBytes = 0;	
	f_read(pHandle, data, length, &recBytes);
}

int8_t ImageSeek(void *pHandle, uint32_t u32FilePos)
{
	f_lseek(pHandle, (FSIZE_t)u32FilePos);
	return 0;
}

int8_t ImagePrepare(void *pDisp, pifINFO_t *pifInfo)
{
	enum RA8876_BTE_S0_color bps = BTE_S0_Color_8bpp;
	uint32_t dispAddress;
	char *tBuf;
	
	dispAddress = RA8876_readReg(RA8876_CVSSA0);
	dispAddress |= (uint32_t)RA8876_readReg(RA8876_CVSSA1) << 8;
	dispAddress |= (uint32_t)RA8876_readReg(RA8876_CVSSA2) << 16;
	dispAddress |= (uint32_t)RA8876_readReg(RA8876_CVSSA3) << 24;	
	
	gs_log_f(GLOG_INFO, "[PIF] Image Pixels: %"PRIu16"w x %"PRIu16"h", pifInfo->imageWidth, pifInfo->imageHeight);
	gs_log_f(GLOG_INFO, "[PIF] File Size: %"PRIu32" Bytes - Image size: %"PRIu32 " Bytes", pifInfo->fileSize, pifInfo->imageSize);
	switch(pifInfo->imageType)
	{
		case PIF_TYPE_RGB332:
			tBuf = "RGB332";
			bps = BTE_S0_Color_8bpp;
			break;
		case PIF_TYPE_IND8:
			tBuf = "IND8";
			bps = BTE_S0_Color_8bpp;
			break;
		case PIF_TYPE_RGB565:
			tBuf = "RGB565";
			bps = BTE_S0_Color_16bpp;
			break;
		case PIF_TYPE_IND16:
			tBuf = "IND16";
			bps = BTE_S0_Color_16bpp;
			break;
		case PIF_TYPE_RGB16C:
			tBuf = "RGB16C";
			bps = BTE_S0_Color_16bpp;
			break;
		case PIF_TYPE_MONO:
			tBuf = "MONO";
			bps = BTE_S0_Color_16bpp;
			break;
		case PIF_TYPE_IND24:
			tBuf = "IND24";
			bps = BTE_S0_Color_24bpp;
			break;
		case PIF_TYPE_RGB888:
			tBuf = "RGB888";
			bps = BTE_S0_Color_24bpp;
			break;
		default:
			gs_log_f(GLOG_ERROR, "[PIF] Invalid image type! %"PRIu8, pifInfo->imageType);
			// Invalid image type, cancel operation
			return 1;
	}
	gs_log_f(GLOG_INFO, "[PIF] Image Type: %s - RLE-Compression: %s", tBuf, (pifInfo->compression == PIF_COMPRESSION_RLE) ? "True" : "False");
	
	RA8876_BTE_RawDataUpload(dispAddress, pifInfo->startX, pifInfo->startY, pifInfo->imageWidth, pifInfo->imageHeight, bps);
	return 0;
}

void ImageWrite(void *pDisp, pifINFO_t *pifInfo, uint32_t pixel)
{
	switch (pifInfo->imageType)
	{
		case PIF_TYPE_IND24:
		case PIF_TYPE_RGB888:
			RA8876_writeData(pixel);
			pixel >>= 8;		
		case PIF_TYPE_RGB565:
		case PIF_TYPE_IND16:
		case PIF_TYPE_RGB16C:
		case PIF_TYPE_MONO:
			RA8876_writeData(pixel);
			pixel >>= 8;
		case PIF_TYPE_RGB332:
		case PIF_TYPE_IND8:
			RA8876_writeData(pixel);
	}
}

pifPAINT_t pifDec;
pifIO_t pifIO;
pifHANDLE_t pifHandler;

static void cli_cmd_pif(uint8_t argc, char *argv[])
{
	uint32_t x0, y0;
	uint32_t msWait;
	if (argc > 3)
	{
		x0 = atoi(argv[1]);
		y0 = atoi(argv[2]);
		
		if ((x0 > RA8876_WIDTH) || (y0 > RA8876_HEIGHT))
		{
			gshell_putString_f("Coordinates too large!\r\n");
			return;
		}
		
		msWait = gf_millis();
		pif_OpenAndDisplay(&pifHandler, argv[3], x0, y0);
		msWait = gf_millis() - msWait;
		gshell_printf_f("Image loaded in %"PRIu32" milliseconds!\r\n", msWait);
	}
	else
	{
		gshell_putString_f("PIF Image Display Tester\r\n"
		"Usage:\r\n"
		" pif <x> <y> <image.pif>   - Display a PIF Image at position x/y\r\n");
	}
}

static gshell_cmd_t cmd_pif = {
	G_XARR("pif"),
	cli_cmd_pif,
	G_XARR("Prototype function to display PIF Images"),
	NULL
};

uint8_t ColorTableBuffer[32];
void initPIF(void)
{
	pif_createPainter(&pifDec, &ImagePrepare, &ImageWrite, NULL, NULL, ColorTableBuffer, sizeof(ColorTableBuffer));
	pif_createIO(&pifIO, &ImageOpen, &ImageClose, &ImageReadB, &ImageSeek);
	pif_createPIFHandle(&pifHandler, &pifIO, &pifDec);
	
	// UART Konsolenbefehl regristrieren 
	gshell_register_cmd(&cmd_pif);
}

void mcuboard_init(void)
{
	// Initialisiere erst den Mikrokontroller:
	_mcu_init();
	STATLED_BUSY();
		
	// Alle Schnittstellen initialisieren und Strukturen vorbereiten
	// Serielle Schnitstelle initialisieren:
	uart_init(&(mcuBoard->uartConsole), &USARTE0, UART_DEFAULT_BSCALE_FACTOR, 
			UART_CALCULATE_BSEL_FACTOR(UART_BAUD_RATE), 0, SERIAL_8N1, 
			receiveBuffer, UART_BUFFER_SIZE, transmitBuffer, UART_BUFFER_SIZE);
	// I2C-Bus initialisieren
	TWI_MasterInit(&(mcuBoard->twiMaster), &TWIE, TWI_MASTER_INTLVL_LO_gc, TWI_BAUD(F_CPU, TWI_SPEED));
	// usartSPI für den FlashChip initialisieren
	usartSPI_init(&(mcuBoard->uSPI), &USARTD0, 0, 0);
	usartSPI_port(&(mcuBoard->uSPI), &PORTD, PIN4_bm, 1);
	// Hardware-SPI (für die externen Boards) vorinitialisieren
	hwSPI_init(&SPI_INTERF);
	hwSPI_setClockDivider(&SPI_INTERF, hwSPI_CLOCK_DIV128);
	hwSPI_setBitOrder(&SPI_INTERF, hwSPI_MSBFIRST);
	hwSPI_setDataMode(&SPI_INTERF, hwSPI_MODE0);
	
	// Touch-Struktur einrichten
	tp_initStruct(&(mcuBoard->dispTouch), RA8876_WIDTH, RA8876_HEIGHT, &(mcuBoard->twiMaster),
			&TWI_InterfacePrepare,&TWI_InterfaceSendBytes,&TWI_InterfaceGetBytes, &TWI_InterfaceFinish);
	// Porterweiterung-Struktur einrichten
	mcp23x_initStruct(&(mcuBoard->ioExp), MCP23X08, 0, &(mcuBoard->twiMaster), &TWI_InterfacePrepare,
			&TWI_InterfaceSendBytes, &TWI_InterfaceGetBytes, 0, &TWI_InterfaceFinish);
	// FlashChip-Struktur einrichten
	flash_configInterface(&(mcuBoard->mainFlash), &(mcuBoard->uSPI), &usartSPI_InterfacePrepare,
			&usartSPI_InterfaceSendBytes,&usartSPI_InterfaceTransceiveBytes,
			&usartSPI_InterfaceGetBytes,&usartSPI_InterfaceFinish);
	// Echtzeituhr-Struktur einrichten
	rv3028_initStruct(&(mcuBoard->rtcClk), &(mcuBoard->twiMaster), &TWI_InterfacePrepare,
			&TWI_InterfaceSendBytes,&TWI_InterfaceGetBytes, &TWI_InterfaceFinish);
	// Millis-timer aktivieren
	gf_init();
	
	// Dateimodem initialisieren
	file_modem_init(&fm_recByte, &fm_sendByte, &fm_flushRx);
	
	// UART Konsole aktivieren
	gshell_init(gsh_putc, gf_millis);
	
	_dispInit();
	gshell_setPromt(1);
	gshell_setActive(1);
	
	RA8876_displayOnOff(1);
	
	gs_log_f(GLOG_INFO, "Initialising the MCU board...");
	/* Porterweiterung initialisieren */
	_initInternalPortExpander(&(mcuBoard->ioExp));
	/* Echtzeituhr & interne Zeit initialisieren */
	_initRTC(&(mcuBoard->rtcClk));
	/* Touchdisplay initialisieren */
	_initTouch(&(mcuBoard->dispTouch));
	/* Speicher & Dateisystem initialisieren */
	_initMemFS(&(mcuBoard->mainFlash), &(mcuBoard->flashBuffer), &(mcuBoard->fs));
	/* Grafik-Bibliothek für Bilder initialisieren */
	initPIF();
		
	RA8876_CanvasStartAddr(LAYER_0);
	RA8876_MainWindowStartAddress(LAYER_0);
	if (pif_OpenAndDisplay(&pifHandler, "0:/IMAGES/LOGOFULL.PIF", 0, 0))
	{
		RA8876_setTextCoords(0, 0);
		RA8876_print_PROGMEM("Picture PIF Error!");
	}
	
	gs_log_f(GLOG_OK, "Mikrokontroller-Platine erfolgreich initialisiert!");	
	STATLED_FREE();
}

void mcu_processShell()
{
	uint16_t ch = uart_getc(&(mcuBoard->uartConsole));
	if ((ch >> 8) == UART_DATA_AVAILABLE)
	{
		gshell_CharReceived((char)ch);
	}
}