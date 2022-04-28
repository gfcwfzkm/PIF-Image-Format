/*
 * PIF Test Enviroment
 *
 * MCU:		atxmega384C3
 * DISPLAY:	RA8876
 * MEMORY:	W25Q512JV
 *
 * Created: 08.03.2022
 * Author : gfcwfzkm
 */

#include "main.h"

static microcontrollerBoard_t _mcuBoard;
microcontrollerBoard_t *mcuBoard = &_mcuBoard;

int main(void)
{
	/* Millis Tick */
	uint32_t u32_msTick;
	/* Software delay for pseudo / cooperative multitasking */
	gf_delay_t gfd_shell	= {.callEveryTime = GFD_COMMANDINTERFACE};
		
	/* Initialise Microcontroller Board */
	mcuboard_init();
	
    while (1) 
    {
		/* Get current ms-tick */
		u32_msTick = gf_millis();
		
		if (gf_TimeCheck(&gfd_shell, u32_msTick))
		{
			/* Run LED */
			STATLED_BUSY();
			mcu_processShell();
		}
		else
		{
			STATLED_FREE();
		}
    }
}