#ifndef GFC32VF103_SPI_H
#define GFC32VF103_SPI_H

#include "gd32vf103.h"

enum HWSPI_ORDER {
	HWSPI_ORDER_MSBFIRST = 0,	// MSB First
	HWSPI_ORDER_LSBFIRST = 1	// LSB First
};	

enum HWSPI_MODE {
	HWSPI_MODE_0 = 0,	// CKPL = 0, CKPH = 0
	HWSPI_MODE_1 = 1,	// CKPL = 0, CKPH = 1
	HWSPI_MODE_2 = 2,	// CKPL = 1, CKPH = 0
	HWSPI_MODE_3 = 3	// CKPL = 1, CKPH = 1
};

enum HWSPI_CSPOL{
	HWSPI_CSPOL_HIGH = 0,	// Chip Select Active High
	HWSPI_CSPOL_LOW = 1		// Chip Select Active Low
};

enum HWSPI_CLKDIV {
	HWSPI_CLKDIV_2 = 0,
	HWSPI_CLKDIV_4 = 1,
	HWSPI_CLKDIV_8 = 2,
	HWSPI_CLKDIV_16 = 3,
	HWSPI_CLKDIV_32 = 4,
	HWSPI_CLKDIV_64 = 5,
	HWSPI_CLKDIV_128 = 6,
	HWSPI_CLKDIV_256 = 7
};

typedef struct {
	uint32_t spi_p;
	uint32_t port_p;
	uint16_t cs_pins;
	enum HWSPI_CSPOL activelow:1;
	enum HWSPI_MODE mode:2;
	enum HWSPI_ORDER lsb_first:1;
	enum HWSPI_CLKDIV clkdiv:3;
}SPI_Master_t;

void hwSPI_init(uint32_t spi_p);

void hwSPI_close(uint32_t spi_p);

void hwSPI_setBitOrder(uint32_t spi_p, enum HWSPI_ORDER bitOrder);

void hwSPI_setDataMode(uint32_t spi_p, enum HWSPI_MODE spimode);

void hwSPI_setClockDivider(uint32_t spi_p, enum HWSPI_CLKDIV clockdiv);

uint8_t hwSPI_transfer(uint32_t spi_p, uint8_t data);

void hwSPI_SetupInterface(SPI_Master_t *spi_master, uint32_t spi_p, uint32_t port_p, uint16_t cs_pins);
uint8_t hwSPI_InterfacePrepare(void *intSPI);
uint8_t hwSPI_InterfaceSendBytes(void *intSPI,
						uint8_t addr,
						uint8_t *buf_ptr,
						uint16_t buf_len);
uint8_t hwSPI_InterfaceTransceiveBytes(void *intSPI,
						uint8_t addr,
						uint8_t *buf_ptr,
						uint16_t buf_len);
uint8_t hwSPI_InterfaceGetBytes(void *intSPI,
						uint8_t addr,
						uint8_t *buf_ptr,
						uint16_t buf_len);
uint8_t hwSPI_InterfaceFinish(void *intSPI);

#endif