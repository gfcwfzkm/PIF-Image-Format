/*
 * GD32VF103 e-paper I/O Definition
 */

#ifndef IO_H
#define IO_H

#include "gd32vf103.h"
#include "gfc32vf103_spi.h"
#include "spd1656.h"
#include "pifdec.h"
#include "redpaz.h"

/* SPI Display - all on PORTB */
#define SPI_MOSI	GPIO_PIN_15
#define SPI_MISO	GPIO_PIN_14
#define SPI_SCK		GPIO_PIN_13
#define DISPLAY_CS	GPIO_PIN_9	
#define DISPLAY_DC	GPIO_PIN_8
#define DISPLAY_RST	GPIO_PIN_7
#define DISPLAY_BSY	GPIO_PIN_6

/* Onboard-LED */
#define ONBOARDLED	GPIO_PIN_13

void initMCU(void);
int8_t prepStuff(void *p_Display, pifINFO_t* p_pifInfo);
void drawStuff(void *p_Display, pifINFO_t* p_pifInfo, uint32_t pixel);
int8_t finishStuff(void *p_Display, pifINFO_t* p_pifInfo);
void* openStuff(const char* pc_filePath, int8_t *fileError);
int8_t closeStuff(void *p_file);
void readStuff(void *p_file, uint8_t *p_buffer, uint8_t size);
int8_t seekStuff(void *p_file, uint32_t offset);
void epaper_setPin(void* epd, spd1656_pin epd_pin, uint8_t ioState);
uint8_t epaper_checkBusy(void* epd);
void delay_ms(uint32_t count);
uint32_t getMS(void);


#endif