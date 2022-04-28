/**
 * @file hwSPI.c
 * @brief Funktionsdatei
 */ 

#include "hwSPI.h"

void hwSPI_init(SPI_t *spi_p)
{
	spi_p->CTRL = SPI_ENABLE_bm | SPI_MASTER_bm;
}

void hwSPI_close(SPI_t *spi_p)
{
	spi_p->CTRL &=~ SPI_ENABLE_bm;
}

void hwSPI_setBitOrder(SPI_t *spi_p, uint8_t bitOrder)
{
	if (bitOrder==hwSPI_LSBFIRST)
	{
		spi_p->CTRL |= SPI_DORD_bm;
	}
	else
	{
		spi_p->CTRL &=~ SPI_DORD_bm;
	}
}

void hwSPI_setDataMode(SPI_t *spi_p, uint8_t spimode)
{
	spi_p->CTRL = (spi_p->CTRL & ~SPI_MODE_gm) | spimode;
}

void hwSPI_setClockDivider(SPI_t *spi_p, uint8_t rate)
{
	uint8_t temp_reg = spi_p->CTRL & ~(SPI_PRESCALER_gm | SPI_CLK2X_bm);
	
	temp_reg |= rate;
	
	spi_p->CTRL = temp_reg;
}

uint8_t hwSPI_transfer(SPI_t *spi_p, uint8_t _data)
{
	spi_p->DATA = _data;
	while(!(spi_p->STATUS & SPI_IF_bm));
	return spi_p->DATA;
}

uint8_t hwSPI_InterfacePrepare(void *intSPI)
{
	SPI_Master_t *hwspi = (SPI_Master_t*)intSPI;
	// Check if SPI is even initialized
	if (!(hwspi->spi_p->CTRL & (SPI_ENABLE_bm | SPI_MASTER_bm)))
	{
		return 1;
	}
	hwSPI_setDataMode(hwspi->spi_p, hwspi->mode);
	hwSPI_setBitOrder(hwspi->spi_p, hwspi->lsb_first);
	// chip-select
	if (hwspi->activelow)
	{
		hwspi->port->OUTCLR = hwspi->cs_pins;
	}
	else
	{
		hwspi->port->OUTSET = hwspi->cs_pins;
	}
	return 0;
}

uint8_t hwSPI_InterfaceSendBytes(void *intSPI, uint8_t addr,
								uint8_t *buf_ptr, uint16_t buf_len)
{
	SPI_Master_t *hwspi = (SPI_Master_t*)intSPI;
	uint16_t byteCnt;
	
	if (addr != 0)	hwSPI_transfer(hwspi->spi_p, addr);
	
	for(byteCnt = 0; byteCnt < buf_len; byteCnt++)
	{
		hwSPI_transfer(hwspi->spi_p,buf_ptr[byteCnt]);
	}
	
	return 0;
}

uint8_t hwSPI_InterfaceTransceiveBytes(void *intSPI, uint8_t addr,
								uint8_t *buf_ptr, uint16_t buf_len)
{
	SPI_Master_t *hwspi = (SPI_Master_t*)intSPI;
	uint16_t byteCnt;
	
	if (addr != 0)	hwSPI_transfer(hwspi->spi_p, addr);
	
	for(byteCnt = 0; byteCnt < buf_len; byteCnt++)
	{
		buf_ptr[byteCnt] = hwSPI_transfer(hwspi->spi_p,buf_ptr[byteCnt]);
	}
	
	return 0;
}

uint8_t hwSPI_InterfaceGetBytes(void *intSPI, uint8_t addr,
								uint8_t *buf_ptr, uint16_t buf_len)
{
	SPI_Master_t *hwspi = (SPI_Master_t*)intSPI;
	uint16_t byteCnt;
	
	if (addr != 0)	hwSPI_transfer(hwspi->spi_p, addr);
	
	for(byteCnt = 0; byteCnt < buf_len; byteCnt++)
	{
		buf_ptr[byteCnt] = hwSPI_transfer(hwspi->spi_p,0);
	}
	
	return 0;
}

uint8_t hwSPI_InterfaceFinish(void *intSPI)
{
	SPI_Master_t *hwspi = (SPI_Master_t*)intSPI;
	
	// chip-select
	if (hwspi->activelow)
	{
		hwspi->port->OUTSET = hwspi->cs_pins;
	}
	else
	{
		hwspi->port->OUTCLR = hwspi->cs_pins;
	}
	
	return 0;
}