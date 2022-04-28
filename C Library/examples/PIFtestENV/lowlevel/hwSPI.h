/**
 * @mainpage Hardware-SPI Bibliothek für Atmel AVR Mikrocontroller
 * @brief Die Bibliothek erlaubt die einfache Anwendung der Hardware-SPI
 * Peripherie des Atmel Microcontrollers und kann nachträglich vom
 * Anwender mit einer Interrupt-Routine erweitert werden.
 *
 * @version 1.0
 * -Veröffentlichung der Bibliothek
 *
 * @author Pascal Gesell
 * @date 25.09.2015
 */

/**
 * @file hwSPI.h
 * @brief Headerdatei der Hardware-SPI Bibliothek
 */

#ifndef HWSPI_H_
#define HWSPI_H_

#include <avr/io.h>

/**
 * @brief SPI-Takt = CPU-Takt (Fosc) durch 4 teilen
 */
#define hwSPI_CLOCK_DIV4	SPI_PRESCALER_DIV4_gc
/**
 * @brief SPI-Takt = CPU-Takt (Fosc) durch 16 teilen
 */
#define hwSPI_CLOCK_DIV16	SPI_PRESCALER_DIV16_gc
/**
 * @brief SPI-Takt = CPU-Takt (Fosc) durch 64 teilen
 */
#define hwSPI_CLOCK_DIV64	SPI_PRESCALER_DIV64_gc
/**
 * @brief SPI-Takt = CPU-Takt (Fosc) durch 28 teilen
 */
#define hwSPI_CLOCK_DIV128	SPI_PRESCALER_DIV128_gc
/**
 * @brief SPI-Takt = CPU-Takt (Fosc) durch 2 teilen
 */
#define hwSPI_CLOCK_DIV2	SPI_PRESCALER_DIV4_gc | SPI_CLK2X_bm
/**
 * @brief SPI-Takt = CPU-Takt (Fosc) durch 8 teilen
 */
#define hwSPI_CLOCK_DIV8	SPI_PRESCALER_DIV16_gc | SPI_CLK2X_bm
/**
 * @brief SPI-Takt = CPU-Takt (Fosc) durch 32 teilen
 */
#define hwSPI_CLOCK_DIV32	SPI_PRESCALER_DIV64_gc | SPI_CLK2X_bm

/**
 * @brief SPI-Modus: CPOL: Rising, CPHA: Sample
 */
#define hwSPI_MODE0	SPI_MODE_0_gc
/**
 * @brief SPI-Modus: CPOL: Rising, CPHA: Setup
 */
#define hwSPI_MODE1	SPI_MODE_1_gc
/**
 * @brief SPI-Modus: CPOL: Falling, CPHA: Sample
 */
#define hwSPI_MODE2	SPI_MODE_2_gc
/**
 * @brief SPI-Modus: CPOL: Falling, CPHA: Setup
 */
#define hwSPI_MODE3	SPI_MODE_3_gc

/**
 * @brief MODE Mask
 */
#define hwSPI_MODE_MASK		SPI_MODE_3_gc
/**
 * @brief Clock Mask
 */
#define hwSPI_CLOCK_MASK	SPI_PRESCALER_gm | SPI_CLK2X_bm

/**
 * @brief Definition des LSB-Makros
 */
#define hwSPI_LSBFIRST	1

/**
 * @brief Definition des MSB-Makros
 */
#define hwSPI_MSBFIRST	0

typedef struct SPI_Master {
	SPI_t *spi_p;			// SPI Peripheral Instance
	PORT_t *port;			// Chip-Select-Port
	uint8_t cs_pins;		// Chip-Select-Pins
	uint8_t activelow:1;	// Active Low or Active High
	uint8_t mode:4;			// SPI Mode
	uint8_t lsb_first:1;	// 1 if LSB is sent first
}SPI_Master_t;

/**
 * @brief Hardware-SPI Schnittstelle initialisieren
 *
 * Initialisiert die SPI-Schnittstelle und stellt diesen als Master ein.
 * \a hwSPI_configurePins muss zuerst aufgerufen werden!
 */
void hwSPI_init(SPI_t *spi_p);

/**
 * @brief SPI-Schnittstelle beenden/ausschalten
 *
 * Deaktiviert die SPI-Schnittstelle. Die eingestellten SPI-Pins bleiben weiterhin
 * gespeichert.
 */
void hwSPI_close(SPI_t *spi_p);

/**
 * @brief Datenübertragung MSB oder LSB
 *
 * Stellt die SPI-Hardware darauf ein, ob das LSB oder MSB
 * zuerst gesendet werden soll.
 * @param bitOrder Stellt die Bitreihenfolge ein (MSB oder LSB First)
 */
void hwSPI_setBitOrder(SPI_t *spi_p, uint8_t bitOrder);

/**
 * @brief Datenmodus setzten
 *
 * Stellt den Datenmodus der SPI-Peripherie ein. Siehe dazu
 * das Datenblatt für weitere Infos.
 * @param mode SPI-Modus (z.B. hwSPI_MODE0)
 */
void hwSPI_setDataMode(SPI_t *spi_p, uint8_t spimode);

/**
 * @brief Taktvorteiler einstellen
 *
 * Stellt den Taktvorteiler ein, womit der CPU-Takt
 * für den SPI-BUS vorgeteilt wird (mind.: 2, max.: 128)
 * @param rate Taktvorteiler (z.B. hwSPI_CLOCK_DIV2)
 */
void hwSPI_setClockDivider(SPI_t *spi_p, uint8_t rate);

/**
 * @brief Daten senden und empfangen
 *
 * Einfache Funktion zum senden und auslesen
 * von Daten in/aus dem SPI-BUS.
 * \n Beispiel: \n \code{.c}
 * hwSPI_transfer(0xAA);					// Wert 0xAA in den BUS schreiben/senden
 * uint8_t _getData = hwSPI_transfer(0);	// Byte aus dem BUS / Empfangsregister lesen
 * \endcode 
 * @param _data Das zu sendende Byte
 * @return Das ausgelesene Byte
 */
uint8_t hwSPI_transfer(SPI_t *spi_p, uint8_t _data);

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

#endif /* HWSPI_H_ */