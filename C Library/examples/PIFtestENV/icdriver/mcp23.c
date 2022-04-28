/*
 * mcp23.c
 *
 * Created: 10.01.2021
 *  Author: gfcwfzkm
 */ 

#include "mcp23.h"
#include <util/delay.h>

#define MCP23_BASE_ADDR		0x40
#define MCP23_CMD_READ		0x01
#define MCP23_CMD_WRITE		0x00
#define MCP23_BUF_SIZE_RD	1
#define MCP23_BUF_SIZE_WR	2

uint8_t mcp_Buffer[MCP23_BUF_SIZE_RD + MCP23_BUF_SIZE_WR];

void mcp23x_initStruct(mcp23x_t* inst, enum mcp23x_type mcp_type,
	uint8_t mcp_subAddr, void *ioInterface, uint8_t (*startTransaction)(void*),
	uint8_t (*sendBytes)(void*,uint8_t,uint8_t*,uint16_t),
	uint8_t (*getBytes)(void*,uint8_t,uint8_t*,uint16_t),
	uint8_t (*transceiveBytes)(void*,uint8_t,uint8_t*,uint16_t),
	uint8_t (*endTransaction)(void*))
{
	inst->mcp_type = mcp_type & 0x1;
	inst->mcp_addr = mcp_subAddr & 0x7;
	inst->err = MCP23_NO_ERROR;
	inst->ioInterface = ioInterface;
	inst->startTransaction = startTransaction;
	inst->sendBytes = sendBytes;
	inst->transceiveBytes = transceiveBytes;
	inst->getBytes = getBytes;
	inst->endTransaction = endTransaction;
}

enum mcp23x_error mcp23x_initChip(mcp23x_t* inst)
{
	inst->err = MCP23_NO_ERROR;
	// Config MCP first as Input
	mcp23x_writeReg(inst, MCP_IODIRA, 0xFF);
	mcp23x_writeReg(inst, MCP_OLATA, 0x00);
	
	// BANK bit can be written on the MCP23008, it just won't have any effects / isn't implemented.
	if (inst->mcp_type == MCP23X17)
	{
		// Assume that the MCP just has been resetted, and that it's at IOCON.BANK = 0, so set it to one
		mcp23x_writeReg(inst, MCP23X17_IOCON_STARTUP, MCP_IOCON_DEFAULT);
		inst->err |= mcp23x_configure(inst, MCP_IOCON_DEFAULT);
		// Check if we truly just got out of a reset, else fix it to the right registers
		if (mcp23x_readReg(inst, MCP_OLATA) != 0)
		{
			mcp23x_writeReg(inst, MCP_OLATA, 0x00);			
		}
	}
	else
	{
		inst->err |= mcp23x_configure(inst, MCP_IOCON_DEFAULT);
	}
	
	if (inst->mcp_type == MCP23X17)
	{
		// If 16-IO PortExpander, enable Register Bank in IOCON
		mcp23x_writeReg(inst, MCP_IODIRB, 0xFF);
		mcp23x_writeReg(inst, MCP_OLATB, 0x00);
	}
	
	return inst->err;
}

void mcp23x_writePort(mcp23x_t* inst, enum mcp23x_port _prt, uint8_t value)
{
	inst->err = MCP23_NO_ERROR;
	
	if (_prt == MCP23_PORTA)
	{
		mcp23x_writeReg(inst, MCP_OLATA, value);
	}
	else if ( (_prt == MCP23_PORTB) && (inst->mcp_type==MCP23X17) )
	{
		mcp23x_writeReg(inst, MCP_OLATB, value);
	}
	else
	{
		inst->err = MCP23_NOREG;
	}
}

void mcp23x_bitSetPort(mcp23x_t* inst, enum mcp23x_port _prt, uint8_t value)
{
	uint8_t temp_reg;
	inst->err = MCP23_NO_ERROR;
	
	if (_prt == MCP23_PORTA)
	{
		temp_reg = mcp23x_readReg(inst, MCP_OLATA);
		temp_reg |= value;
		mcp23x_writeReg(inst, MCP_OLATA, temp_reg);
	}
	else if ( (_prt == MCP23_PORTB) && (inst->mcp_type==MCP23X17) )
	{
		temp_reg = mcp23x_readReg(inst, MCP_OLATB);
		temp_reg |= value;
		mcp23x_writeReg(inst, MCP_OLATB, temp_reg);
	}
	else
	{
		inst->err = MCP23_NOREG;
	}
}

void mcp23x_bitClearPort(mcp23x_t* inst, enum mcp23x_port _prt, uint8_t value)
{
	uint8_t temp_reg;
	inst->err = MCP23_NO_ERROR;
	
	if (_prt == MCP23_PORTA)
	{
		temp_reg = mcp23x_readReg(inst, MCP_OLATA);
		temp_reg &=~ value;
		mcp23x_writeReg(inst, MCP_OLATA, temp_reg);
	}
	else if ( (_prt == MCP23_PORTB) && (inst->mcp_type==MCP23X17) )
	{
		temp_reg = mcp23x_readReg(inst, MCP_OLATB);
		temp_reg &=~ value;
		mcp23x_writeReg(inst, MCP_OLATB, temp_reg);
	}
	else
	{
		inst->err = MCP23_NOREG;
	}
}

void mcp23x_bitTogglePort(mcp23x_t* inst, enum mcp23x_port _prt, uint8_t value)
{
	uint8_t temp_reg;
	inst->err = MCP23_NO_ERROR;
	
	if (_prt == MCP23_PORTA)
	{
		temp_reg = mcp23x_readReg(inst, MCP_OLATA);
		temp_reg ^= value;
		mcp23x_writeReg(inst, MCP_OLATA, temp_reg);
	}
	else if ( (_prt == MCP23_PORTB) && (inst->mcp_type==MCP23X17) )
	{
		temp_reg = mcp23x_readReg(inst, MCP_OLATB);
		temp_reg ^= value;
		mcp23x_writeReg(inst, MCP_OLATB, temp_reg);
	}
	else
	{
		inst->err = MCP23_NOREG;
	}
}

uint8_t mcp23x_readPort(mcp23x_t* inst, enum mcp23x_port _prt)
{
	uint8_t temp_reg = 0;
	inst->err = MCP23_NO_ERROR;
	
	if (_prt == MCP23_PORTA)
	{
		temp_reg = mcp23x_readReg(inst, MCP_GPIOA);
	}
	else if ( (_prt == MCP23_PORTB) && (inst->mcp_type==MCP23X17) )
	{
		temp_reg = mcp23x_readReg(inst, MCP_GPIOB);
	}
	else
	{
		inst->err = MCP23_NOREG;
	}
	
	return temp_reg;
}

void mcp23x_dirPort(mcp23x_t* inst, enum mcp23x_port _prt, uint8_t value)
{
	inst->err = MCP23_NO_ERROR;
	
	if (_prt == MCP23_PORTA)
	{
		mcp23x_writeReg(inst, MCP_IODIRA, value);
	}
	else if ( (_prt == MCP23_PORTB) && (inst->mcp_type==MCP23X17) )
	{
		mcp23x_writeReg(inst, MCP_IODIRB, value);
	}
	else
	{
		inst->err = MCP23_NOREG;
	}
}

void mcp23x_invInPolarity(mcp23x_t* inst, enum mcp23x_port _prt, uint8_t value)
{
	inst->err = MCP23_NO_ERROR;
	
	if (_prt == MCP23_PORTA)
	{
		mcp23x_writeReg(inst, MCP_IPOLA, value);
	}
	else if ( (_prt == MCP23_PORTB) && (inst->mcp_type==MCP23X17) )
	{
		mcp23x_writeReg(inst, MCP_IPOLB, value);
	}
	else
	{
		inst->err = MCP23_NOREG;
	}
}

void mcp23x_setPullUp(mcp23x_t* inst, enum mcp23x_port _prt, uint8_t value)
{
	inst->err = MCP23_NO_ERROR;
	
	if (_prt == MCP23_PORTA)
	{
		mcp23x_writeReg(inst, MCP_GPPUA, value);
	}
	else if ( (_prt == MCP23_PORTB) && (inst->mcp_type==MCP23X17) )
	{
		mcp23x_writeReg(inst, MCP_GPPUA, value);
	}
	else
	{
		inst->err = MCP23_NOREG;
	}
}

enum mcp23x_error mcp23x_configure(mcp23x_t* inst, enum mcp23x_iocon configVal)
{
	volatile uint8_t IOCON_readback;
	inst->err = MCP23_NO_ERROR;
	
	mcp23x_writeReg(inst, MCP_IOCON, configVal);
	IOCON_readback = mcp23x_readReg(inst, MCP_IOCON);
		
	if ((IOCON_readback == 0xFF) || (IOCON_readback == 0x00))
	{
		inst->err = MCP23_IOERR;
	}
	else if ((inst->mcp_type == MCP23X17) && (IOCON_readback != configVal))
	{
		inst->err = MCP23_IOERR;
	}
	else if ((inst->mcp_type == MCP23X08) && (IOCON_readback != (configVal & 0x3E)))
	{		
		inst->err = MCP23_IOERR;
	}
	
	return inst->err;
}

uint8_t mcp23x_writeReg(mcp23x_t* inst, uint8_t regAddr, uint8_t value)
{
	enum mcp23x_error tmp_err = MCP23_NO_ERROR;
	uint8_t mcp_Address;
	
	mcp_Address = MCP23_BASE_ADDR | MCP23_CMD_WRITE | (inst->mcp_addr << 1);
	
	
	mcp_Buffer[0] = regAddr;
	mcp_Buffer[1] = value;
	
	tmp_err |= inst->startTransaction(inst->ioInterface);
	/* Start Transaction (setting Address, reading Register)
	 * but only if the driver doesn't respond with an error (anything non-0) */
	if (!tmp_err)
	{
		if (inst->sendBytes != 0)
		{
			tmp_err |= inst->sendBytes(inst->ioInterface,
				mcp_Address, mcp_Buffer, MCP23_BUF_SIZE_WR);
		}
		else
		{
			tmp_err |= inst->transceiveBytes(inst->ioInterface, mcp_Address, mcp_Buffer, MCP23_BUF_SIZE_WR);			
		}
	}
	// End Transaction, no matter if it failed or was successful
	tmp_err |= inst->endTransaction(inst->ioInterface);
	
	inst->err |= tmp_err;
	return tmp_err;
}

uint8_t mcp23x_readReg(mcp23x_t* inst, uint8_t regAddr)
{
	enum mcp23x_error tmp_err = MCP23_NO_ERROR;
	uint8_t mcp_Address;
	
	mcp_Buffer[0] = mcp_Address = MCP23_BASE_ADDR | MCP23_CMD_READ | (inst->mcp_addr << 1);
	
	mcp_Buffer[1] = regAddr;
	mcp_Buffer[2] = 0x00;
	
	tmp_err |= inst->startTransaction(inst->ioInterface);
	// Start Transaction (setting Address, reading Register)
	// but only if the driver doesn't respond with an error (anything non-0)
	if (!tmp_err)
	{
		if (inst->sendBytes != 0)
		{
			tmp_err |= inst->sendBytes(inst->ioInterface,
				mcp_Address, &mcp_Buffer[1], MCP23_BUF_SIZE_RD);
			tmp_err |= inst->getBytes(inst->ioInterface,
				mcp_Address, mcp_Buffer, MCP23_BUF_SIZE_RD);
		}
		else
		{
			tmp_err |= inst->transceiveBytes(inst->ioInterface,
				0, mcp_Buffer, 3);			
			mcp_Buffer[0] = mcp_Buffer[2];
		}
	}
	// End Transaction, no matter if it failed or was successful
	tmp_err |= inst->endTransaction(inst->ioInterface);
	
	inst->err |= tmp_err;
	return mcp_Buffer[0];
}
