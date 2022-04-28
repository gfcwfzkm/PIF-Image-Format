/**
 * mcp23.h
 * Bibliothek für die I2C und SPI Versionen des MCP23x08 und MCP23x17
 * Created: 10.01.2021
 *  Author: gfcwfzkm
 */

#ifndef MCP23_DRIVER_H_
#define MCP23_DRIVER_H_

#include <inttypes.h>

enum mcp23x_register {
	MCP_IODIRA		= 0x00,
	MCP_IPOLA		= 0x01,
	MCP_GPINTENA	= 0x02,
	MCP_DEFVALA		= 0x03,
	MCP_INTCONA		= 0x04,
	MCP_IOCON		= 0x05,
	MCP_GPPUA		= 0x06,
	MCP_INTFA		= 0x07,
	MCP_INTCAPA		= 0x08,
	MCP_GPIOA		= 0x09,
	MCP_OLATA		= 0x0A,
	MCP_IODIRB		= 0x10,
	MCP_IPOLB		= 0x11,
	MCP_GPINTENB	= 0x12,
	MCP_DEFVALB		= 0x13,
	MCP_INTCONB		= 0x14,
	//MCP_IOCON		= 0x15,	-> same as 0x05
	MCP_GPPUB		= 0x16,
	MCP_INTFB		= 0x17,
	MCP_INTCAPB		= 0x18,
	MCP_GPIOB		= 0x19,
	MCP_OLATB		= 0x1A,
	MCP23X17_IOCON_STARTUP = 0x0A
};

enum mcp23x_type{
	MCP23X08	= 0,
	MCP23X17	= 1
};

enum mcp23x_error {
	MCP23_NO_ERROR	= 0,
	MCP23_IOERR		= 1,
	MCP23_TIMEOUT	= 2,
	MCP23_NOREG		= 3
};

enum mcp23x_port {
	MCP23_PORTA,
	MCP23_PORTB
};

enum mcp23x_iocon{
	MCP23_IOCON_INTPOL	= 0x02,
	MCP23_IOCON_ODR		= 0x04,
	MCP23_IOCON_HAEN	= 0x08,
	MCP23_IOCON_DISSLW	= 0x10,
	MCP23_IOCON_SEQOP	= 0x20,
	MCP23_IOCON_MIRROR	= 0x40,
	MCP23_IOCON_BANK	= 0x80
};

#define MCP_IOCON_DEFAULT		(MCP23_IOCON_BANK | MCP23_IOCON_SEQOP |\
								MCP23_IOCON_HAEN | MCP23_IOCON_ODR)

typedef struct {
	enum mcp23x_type mcp_type:1;		// MCP23x08 (8 I/Os) or MCP23x17 (16 I/Os)
	uint8_t mcp_addr:3;					// Custom sub-address set at it's Adress-Pins
	enum mcp23x_error err:3;			// Check errors here
	void *ioInterface;					// Pointer to the IO/Peripheral Interface library
	// Any return value by the IO interface functions have to return zero when successful or
	// non-zero when not successful.
	uint8_t (*startTransaction)(void*);	// Prepare the IO/Peripheral Interface for a transaction
	uint8_t (*sendBytes)(void*,			// Send data function pointer: InterfacePointer,
						uint8_t,		// Address of the PortExpander (8-Bit Address Format!),
						uint8_t*,		// Pointer to send buffer,
						uint16_t);		// Amount of bytes to send
	uint8_t (*getBytes)(void*,			// Get data function pointer:InterfacePointer,
						uint8_t,		// Address of the PortExpander (8-Bit Address Format!),
						uint8_t*,		// Pointer to receive buffer,
						uint16_t);		// Amount of bytes to receive
	uint8_t (*transceiveBytes)(void*,			// Get data function pointer:InterfacePointer,
						uint8_t,		// Address of the PortExpander (8-Bit Address Format!),
						uint8_t*,		// Pointer to receive buffer,
						uint16_t);		// Amount of bytes to receive
	uint8_t (*endTransaction)(void*);	// Finish the transaction / Release IO/Peripheral
} mcp23x_t;

/* @brief Creating the struct for further processing & controlling
 *
 * Requires the basic I/O driver functions to be passed, as well as the interface
 * needed to use the I/O drivers. If the I2C Variant of the chip is used, pass NULL as
 * value for the transceiveBytes function, if the SPI Variant is used, pass NULL as 
 * value for the sendBytes & getBytes functions.
 *
 * @param inst				pointer to the mcp23x_t struct
 * @param mcp_type			Choose MCP23X08 or MCP23X17
 * @param mcp_subAddr		Pass over the Sub-Address, selectable by the A0-A2 pins
 * @param ioInterface		Pointer to the IO/Interface Library
 * @param startTransaction	FuncPointer: Prepare the IO Interface for a transaction (eg. Chip-Select...)
 * @param sendBytes			FuncPointer: Send x amount of bytes to an optional address
 * @param getBytes			FuncPointer: Get x amount of bytes from an optional address
 * @param transceiveBytes	FuncPointer Send & Get x amount of bytes from an optional address
 * @param endTransaction	FuncPointer: Finish the transaction (eg. Chip-Select, emptying buffer...)
 */
void mcp23x_initStruct(mcp23x_t* inst, enum mcp23x_type mcp_type,
	uint8_t mcp_subAddr, void *ioInterface, uint8_t (*startTransaction)(void*),
	uint8_t (*sendBytes)(void*,uint8_t,uint8_t*,uint16_t),
	uint8_t (*getBytes)(void*,uint8_t,uint8_t*,uint16_t),
	uint8_t (*transceiveBytes)(void*,uint8_t,uint8_t*,uint16_t),
	uint8_t (*endTransaction)(void*));

/* @brief Initialize & check the chip
 *
 * Initializes the chip, sets all pins as Inputs and clears the output-latch registers
 * Also checks if the chip is responding at all, by reading back the configured registers
 * IOCON Register Bits set: BANK + SEQOP + HAEN + ODR (Only SEQOP & ODR is checked in readback)
 *
 * @param inst				pointer to the configured mcp23x_t struct
 * @return					Non-Zero if an error occured
 */
enum mcp23x_error mcp23x_initChip(mcp23x_t* inst);

/* @brief Write a value to the port (OLAT)
 *
 * (Over-)Writes the output latch register of the port
 *
 * @param inst				Pointer to the configured mcp23x_t struct
 * @param _prt				Either MCP23_PORTA or MCP23_PORTB (- only on MCP23X17)
 * @param value				The value to write into
 */
void mcp23x_writePort(mcp23x_t* inst, enum mcp23x_port _prt, uint8_t value);

/* @brief Sets specific output bits
 *
 * Sets specific bits to one without overwriting other bits of the output latch register
 *
 * @param inst				Pointer to the configured mcp23x_t struct
 * @param _prt				Either MCP23_PORTA or MCP23_PORTB (- only on MCP23X17)
 * @param value				The value to set bits to one
 */
void mcp23x_bitSetPort(mcp23x_t* inst, enum mcp23x_port _prt, uint8_t value);

/* @brief Clears specific output bits
 *
 * Clears specific bits to 0 without overwriting other bits of the output latch register
 *
 * @param inst				Pointer to the configured mcp23x_t struct
 * @param _prt				Either MCP23_PORTA or MCP23_PORTB (- only on MCP23X17)
 * @param value				The value to clear the bits to zero
 */
void mcp23x_bitClearPort(mcp23x_t* inst, enum mcp23x_port _prt, uint8_t value);

/* @brief Toggles specific output bits
 *
 * (Over-)Writes the output latch register of the port
 *
 * @param inst				Pointer to the configured mcp23x_t struct
 * @param _prt				Either MCP23_PORTA or MCP23_PORTB (- only on MCP23X17)
 * @param value				The value to write into
 */
void mcp23x_bitTogglePort(mcp23x_t* inst, enum mcp23x_port _prt, uint8_t value);

/* @brief Reads back a port
 *
 * Reads the input register of a port
 *
 * @param inst				Pointer to the configured mcp23x_t struct
 * @param _prt				Either MCP23_PORTA or MCP23_PORTB (- only on MCP23X17)
 * @return					Value of the GPIO (Input Register)
 */
uint8_t mcp23x_readPort(mcp23x_t* inst, enum mcp23x_port _prt);

/* @brief Sets the port's direction register
 *
 * Determines which pin is handled as input or output. A '1' sets the pin to
 * input while a '0' sets it as an output.
 *
 * @param inst				Pointer to the configured mcp23x_t struct
 * @param _prt				Either MCP23_PORTA or MCP23_PORTB (- only on MCP23X17)
 * @param value				Pins to set as In- or Output
 */
void mcp23x_dirPort(mcp23x_t* inst, enum mcp23x_port _prt, uint8_t value);

/* @brief Invert input pin
 *
 * Basically invert the input register's pins
 *
 * @param inst				Pointer to the configured mcp23x_t struct
 * @param _prt				Either MCP23_PORTA or MCP23_PORTB (- only on MCP23X17)
 * @param value				Pins to invert internally
 */
void mcp23x_invInPolarity(mcp23x_t* inst, enum mcp23x_port _prt, uint8_t value);

/* @brief Sets or disables the pullups
 *
 * A '1' enables a (roughly) 100kOhm strong pull-up resistor at the pin
 *
 * @param inst				Pointer to the configured mcp23x_t struct
 * @param _prt				Either MCP23_PORTA or MCP23_PORTB (- only on MCP23X17)
 * @param value				Pins to enable/disable the pullup on
 */
void mcp23x_setPullUp(mcp23x_t* inst, enum mcp23x_port _prt, uint8_t value);

/* @brief Configure (and check) the IOCON register
 *
 * Overwrites the IOCON Register and reads it back to check if it has been written correctly
 * BANK and MIRROR bit are not checked during readback on the MCP23X08
 *
 * @param inst				Pointer to the configured mcp23x_t struct
 * @param configVal			Configure Bits to be written into IOCON
 * @return					Returns non-zero if an error occured
 */
enum mcp23x_error mcp23x_configure(mcp23x_t* inst, enum mcp23x_iocon configVal);

/* @brief Write a value at a register
 *
 * @param inst				Pointer to the configured mcp23x_t struct
 * @param regAddr			Register Address to write at
 * @param value				Register Value
 */
uint8_t mcp23x_writeReg(mcp23x_t* inst, uint8_t regAddr, uint8_t value);

/* @brief Reads a register's value
 *
 * @param inst				Pointer to the configured mcp23x_t struct
 * @param regAddr			Register Address to read from
 * @return					Value read from the register
 */
uint8_t mcp23x_readReg(mcp23x_t* inst, uint8_t regAddr);

#endif /* mcp23x17_DRIVER_H_ */