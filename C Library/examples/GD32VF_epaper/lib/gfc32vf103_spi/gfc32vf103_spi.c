#include "gfc32vf103_spi.h"

void hwSPI_init(uint32_t spi_p)
{
	switch (spi_p)
	{
		case SPI0:
			rcu_periph_clock_enable(RCU_SPI0);
			break;
		case SPI1:
			rcu_periph_clock_enable(RCU_SPI1);
			break;
		case SPI2:
			rcu_periph_clock_enable(RCU_SPI2);
			break;
	}
	spi_i2s_deinit(spi_p);
	SPI_CTL0(spi_p) = SPI_CTL0_MSTMOD | SPI_CTL0_SPIEN | SPI_CTL0_SWNSSEN | SPI_CTL0_SWNSS;
	SPI_I2SCTL(spi_p) = 0;
}

void hwSPI_close(uint32_t spi_p)
{
	spi_i2s_deinit(spi_p);
}

void hwSPI_setBitOrder(uint32_t spi_p, enum HWSPI_ORDER bitOrder)
{
	uint32_t reg = SPI_CTL0(spi_p) & ~SPI_CTL0_LF;
	reg |= bitOrder << 7;
	SPI_CTL0(spi_p) = reg;
}

void hwSPI_setDataMode(uint32_t spi_p, enum HWSPI_MODE spimode)
{
	uint32_t reg = SPI_CTL0(spi_p) & ~(SPI_CTL0_CKPH | SPI_CTL0_CKPL);
	reg |= spimode;
	SPI_CTL0(spi_p) = reg;
}

void hwSPI_setClockDivider(uint32_t spi_p, enum HWSPI_CLKDIV clockdiv)
{
	uint32_t reg = SPI_CTL0(spi_p) & ~SPI_CTL0_PSC;
	reg |= clockdiv << 3;
	SPI_CTL0(spi_p) = reg;
}

uint8_t hwSPI_transfer(uint32_t spi_p, uint8_t data)
{
	while(RESET == spi_i2s_flag_get(spi_p, SPI_FLAG_TBE));
	SPI_DATA(spi_p) = data;
	while(RESET == spi_i2s_flag_get(spi_p, SPI_FLAG_RBNE));
	return SPI_DATA(spi_p);
}

void hwSPI_SetupInterface(SPI_Master_t *spi_master, uint32_t spi_p, uint32_t port_p, uint16_t cs_pins)
{
	uint32_t reg = 0U;

	spi_master->spi_p = spi_p;
	spi_master->port_p = port_p;
	spi_master->cs_pins = cs_pins;
	spi_master->activelow = HWSPI_CSPOL_LOW;
	spi_master->mode = HWSPI_MODE_0;
	spi_master->lsb_first = HWSPI_ORDER_MSBFIRST;
	spi_master->clkdiv = HWSPI_CLKDIV_2;

	reg = SPI_CTL0_MSTMOD | SPI_CTL0_SPIEN | SPI_CTL0_SWNSSEN | SPI_CTL0_SWNSS;
	reg |= spi_master->mode | (spi_master->clkdiv << 3) | (spi_master->lsb_first << 6);
	SPI_CTL0(spi_master->spi_p) = reg;
}

uint8_t hwSPI_InterfacePrepare(void *intSPI)
{
	uint32_t reg = 0U;
	SPI_Master_t *hwspi = (SPI_Master_t*)intSPI;
	// Configure SPI
	reg = SPI_CTL0_MSTMOD | SPI_CTL0_SPIEN | SPI_CTL0_SWNSSEN | SPI_CTL0_SWNSS;
	reg |= hwspi->mode | (hwspi->clkdiv << 3) | (hwspi->lsb_first << 6);
	SPI_CTL0(hwspi->spi_p) = reg;
	// Chip select
	if (hwspi->activelow == HWSPI_CSPOL_LOW)
	{
		GPIO_BC(hwspi->port_p) = hwspi->cs_pins;
	}
	else
	{
		GPIO_BOP(hwspi->port_p) = hwspi->cs_pins;
	}
	return 0;
}

uint8_t hwSPI_InterfaceSendBytes(void *intSPI, uint8_t addr, uint8_t *buf_ptr, uint16_t buf_len)
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

uint8_t hwSPI_InterfaceTransceiveBytes(void *intSPI, uint8_t addr, uint8_t *buf_ptr, uint16_t buf_len)
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

uint8_t hwSPI_InterfaceGetBytes(void *intSPI, uint8_t addr, uint8_t *buf_ptr, uint16_t buf_len)
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
	// Chip select
	if (hwspi->activelow == HWSPI_CSPOL_LOW)
	{
		GPIO_BOP(hwspi->port_p) = hwspi->cs_pins;
	}
	else
	{
		GPIO_BC(hwspi->port_p) = hwspi->cs_pins;
	}
	return 0;
}