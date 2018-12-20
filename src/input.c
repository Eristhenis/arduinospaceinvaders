/*
  input.c  - this file is responsible for the providing
  the functions which poll the input from the buttons
  we are using.

  Note the following pins are used and their purposes
  listed:

  C3, C4 and C5 correspond to button 0, 1 and 2 respectively.

  Author: Group 10 (Michael Nolan)
*/
#include <avr/io.h>

void initButtons(void)
{
	/* Set the 3 pin on port C being used for the
	 * buttons as inputs.
	 *
	 * Obviously we could write this expression
	 * as
	 *   DDRC &=  0xC7;
	 * but in the way it stands it is much easier
	 * to read and understand.
	 */
	DDRC &= ~((1 << 3)|(1 << 4)|(1 << 5));
}

int isButtonDown(int buttonId)
{
	/* Here we simply check if any pin goes low
	 * (we are using pullup resistors on each
	 * switch and thus keeping them high until
	 * depressed).
	 */
	if(buttonId > 3){
		return 0x00;
	}
	return !(PINC & (1 << (3 + buttonId)));
}

int isAnyKeyDown(void)
{
	/* This simply checks if any (of the 3) keys
	 * are depressed - quite useful in our gaming logic.
	 *
	 */
	return !((PINC & 0b00111000) == 0b00111000);
}
