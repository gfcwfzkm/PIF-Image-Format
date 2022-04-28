/*
 * io_map.h
 *
 */ 


#ifndef IO_MAP_H_
#define IO_MAP_H_

/* Port A
 * Paralleler 8-bit Datenbus, geht an das Display & Bustreiber 
 */
#define DATAPORT	PORTA
#define DB0			PIN0_bm
#define DB1			PIN1_bm
#define DB2			PIN2_bm
#define DB3			PIN3_bm
#define DB4			PIN4_bm
#define DB5			PIN5_bm
#define DB6			PIN6_bm
#define DB7			PIN7_bm

/* Port B
 * Address-Port: Board- (PB3-PB6) und IC-Adresse (PB0-PB2), zusammen mit dem MCS (PB7)
 */
#define ADDRPORT	PORTB
#define ADDR_IC0	PIN0_bm
#define ADDR_IC1	PIN1_bm
#define ADDR_IC2	PIN2_bm
#define ADDR_BD0	PIN3_bm
#define ADDR_BD1	PIN4_bm
#define ADDR_BD2	PIN5_bm
#define ADDR_BD3	PIN6_bm
#define ADDR_MCS	PIN7_bm
#define ADDR_BD_BP	3	// Board-Address Bit-Position
#define ADDR_IC_MSK	(ADDR_IC0 | ADDR_IC1 | ADDR_IC2)
#define ADDR_BD_MSK	(ADDR_BD0 | ADDR_BD1 | ADDR_BD2 | ADDR_BD3)

/* Port C
 * Serielle Schnittstellen, gehen zum Bus (SPI & UART), mit zwi Parallel-Port Steuerleitungen
 */
#define SERPORT		PORTC
#define DB_EN		PIN0_bm
#define UART_XCK	PIN1_bm
#define UART_RX		PIN2_bm
#define UART_TX		PIN3_bm
#define DB_RS		PIN4_bm
#define SPI_MOSI	PIN5_bm
#define SPI_MISO	PIN6_bm
#define SPI_SCK		PIN7_bm
#define SPI_INTERF	SPIC

/* Port D
 * Interner SPI Bus, LCD-Steuerleitungen und USB-Datenleitungen
 */
#define MIXPORT		PORTD
#define DB_RW		PIN0_bm
#define ISCK		PIN1_bm
#define IMISO		PIN2_bm
#define IMOSI		PIN3_bm
#define MEM_CS		PIN4_bm
#define LCD_CS		PIN5_bm
#define USB_DP		PIN6_bm
#define USB_DM		PIN7_bm

/* Port E
 * I2C Bus, Debug-UART und div interne Signale
 */
#define MISCPORT	PORTE
#define SDA			PIN0_bm
#define SCL			PIN1_bm
#define DBG_RX		PIN2_bm
#define DBG_TX		PIN3_bm
#define RESET		PIN4_bm
#define TOUCH_IRQ	PIN5_bm
#define PE_IRQ		PIN6_bm
#define RTC_SIG		PIN7_bm

/* Port F
 * LCD Statusleitungen, Drehencoder und Interrupt-Leitungen
 */
#define LCDPORT		PORTF
#define LCD_WAIT	PIN0_bm
#define LCD_IRQ		PIN1_bm
#define LCD_RST		PIN2_bm
#define RE_SW		PIN3_bm
#define RE_A		PIN4_bm
#define RE_B		PIN5_bm
#define IRQH		PIN6_bm
#define IRQM		PIN7_bm

/* Port R
 * "Interrupt Selected" Leitung (active-low) und uC BusyOK Status LED
 */
#define STATPORT	PORTR
#define IRQSEL		PIN0_bm
#define STATLED		PIN1_bm

/* Internes I2C PortErweiterung
 * Kontrolliert einige Reset-Leitungen, USB-Feedback & Low-Level Bus Interrupt
 */

/* MCU Board Port Expander */
#define MCU_PE_IRQL			PIN0_bm	// IN
#define MCU_PE_TOUCH_RST	PIN1_bm	// OUT
#define MCU_PE_WDT_RST		PIN2_bm	// OUT
#define MCU_PE_USBHUB_RST	PIN3_bm	// OUT
#define MCU_PE_VBUS_DET		PIN4_bm	// IN
#define MCU_PE_WDT_FB		PIN5_bm	// IN
#define MCU_PE_UNUSED6		PIN6_bm	//
#define MCU_PE_UNUSED7		PIN7_bm	//
// MicrochipDDR: 0 = Output, 1 = Input
#define MCU_PE_DEFAULT_DDR	(MCU_PE_UNUSED7 | MCU_PE_UNUSED6 | MCU_PE_WDT_FB | MCU_PE_VBUS_DET | MCU_PE_IRQL)
#define MCU_PE_DEFAULT_OUT	(MCU_PE_USBHUB_RST | MCU_PE_TOUCH_RST)

#endif /* IO_MAP_H_ */