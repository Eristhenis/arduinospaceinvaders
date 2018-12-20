/*
  lcd.c  - this file is responsible for the providing
  the functions which implement the interface between
  the Arduino and the LCD display protocol.

  Note the following pins are used and their purposes
  listed:

  All of port D is used for the LCD's 8-bit data bus
  Pins C0 and C1 are used to decide which IC on the
   LCD to talk to
  Pin B1 is used as the LCD chip select pin (i.e. the
   LCD's enable pin, or the strobe pin depending on
   the context)

  Note that the protocol interface implemented here is
  based on the specification outlined by

  http://www.techtoys.com.hk/Displays/JHD12864J/ks0108.pdf

  and

  http://pdfdata.datasheetsite.com/web/40491/KS0108.pdf

  Author: Group 10 (Michael Nolan)
*/
#include <WProgram.h>
#include <avr/pgmspace.h>
#include "lcd.h"

/* Global variables
 *  Although these are defined in data.c
 *
 * 'framebuffer' is 1024 bytes of pre-flush storage space
 * for each frame. By flush we mean the contents of
 * framebuffer will be put out onto the LCD when the
 * lcdRepaint function is used. Under normal circumstances
 * it would be preferred if we used a double buffering
 * approach where we could retain the contents of the
 * screen and switch between that and the next frame.
 * However, in this specific LCD there is only enough
 * memory on each IC to store the active contents of
 * one frame, as such when the strobe line (i.e. the
 * enable pin) is slow enough you can actually watch
 * the image as it gets rendered (8-bits at a time
 * as per 8-bit data bus).
 *
 * Originally (as the original plan was to emulate a
 * Nintendo Gameboy as opposed to Space Invaders) this
 * code was done out in a much quicker assembly equivalent
 * and ran at a very high frame rate (we thought this
 * would be necessary as the majority of process time
 * must be spent doing the actual emulation), but as this
 * redesign only concerns the Space Invaders re-make we
 * have exchanged the assembly instructions for
 * more readable c-code.
 *
 * Also note that 'TEXT' is an array of upper and lower case
 * character bitmaps to be used by the lcdPrintText function
 * (as this LCD does not come pre-loaded with this data unlike
 * most text-based LCDs will do so they must be implemented by
 * the MCU).
 *
 */
extern volatile       unsigned char                              framebuffer[];
extern volatile const unsigned char __attribute__((__progmem__)) TEXT[];

/* Static prototypes */
static void lcdEnableSlow(void);

/* LCD specification specific commands */
#define ON                    1
#define OFF                   0
#define LCD_TURN_ONOFF_CMD(x) (0b00111110 | ( x & 0b1   ))
#define LCD_GOTO_ROW(x)       (0b10111000 | ( x & 0b111 ))
#define LCD_GOTO_ORG()        (0b01000000                )
#define LCD_PIXEL_CMD()       {PORTB |=  (1<<0);}
#define LCD_REGISTER_CMD()    {PORTB &= ~(1<<0);}

/* Macros */
#define ICOFF()            { PORTC |=  (1<<0); PORTC |=  (1<<1); }
#define IC1()              { PORTC |=  (1<<0); PORTC &= ~(1<<1); }
#define IC2()              { PORTC &= ~(1<<0); PORTC |=  (1<<1); }
#define VERY_SHORT_DELAY() { delayMicroseconds(1);               }
#define SHORT_DELAY()      { delayMicroseconds(20);              }
#define LONG_DELAY()       { delay(25);                          }
#define ENABLE_LOW()       { PORTB &= ~ (1<<1);                  }
#define ENABLE_HIGH()      { PORTB |=   (1<<1);                  }

/*
 * In order to keep this code pretty portable and hence usable in
 * other projects, all of the setting up and hardware-specific stuff
 * occurs in the macros so that any changes to the pin layout
 * and ports used can be easily modified by changing only a few of
 * these macros - there should be no hardware dependencies in any
 * non-macro code.
 */
#define SETUP_LCD_PINS(){     \
	DDRD  = 0xff;             \
	DDRB |= (1<<1) | (1<<0);  \
	DDRC |= (1<<0) | (1<<1);  \
}

/*
 * lcdWrite sets the data on the LCD's data-bus by writing
 * the appropriate value to PORTD (which is where we have
 * connected the LCD's 8-bit data-bus to so that we can do
 * this in one simple instruction). Note that, this function
 * will be call extensively when the LCD is being repainted,
 * and the LCD can't (from experimentation) handle the speeds
 * at which the code is capable of sending to the bus, as such
 * a delay has been added to stop the screen from becoming
 * overwhelmed and displaying nonsense. There is commands
 * in place on the LCD which when polled would allow us to
 * obtain the status of how the transfer is coming, but
 * it is much easier (from experimentation again) to delay
 * lcdWrite by about 20 microseconds - this seems to be
 * enough time for the LCD's ICs to copy content from the
 * data-bus to the actual screen (i.e. their own RAM).
 */
#define lcdWrite(x)        { PORTD = x; SHORT_DELAY();           }

/*
 * In order to pass data on the data bus to the LCD we
 * write the data to the data bus (via lcdWrite) and then
 * we must strobe the enable line (which in our board is
 * the pin B1). This function provides that strobe at
 * a fast rate.
 */
static void lcdEnable(void)
{
	ENABLE_HIGH();
	VERY_SHORT_DELAY();
	ENABLE_LOW();
	VERY_SHORT_DELAY();
}

/*
 * Although we cannot find anything in the specification to
 * suggest we should slow down data transfer at the beginning,
 * it is preferred from experience (although not experimentation)
 * that when the devices are first turned on and initialised
 * they should be given some more time to do so. As such,
 * this function is tailored for when we are first communicating
 * with the LCD - which will be to turn it on and ensure it is
 * Initialised. */
static void lcdEnableSlow(void)
{
	ENABLE_HIGH();
	LONG_DELAY();
	ENABLE_LOW();
	LONG_DELAY();
}

/*
 * The following function should be called prior to using
 * any other lcd function - or at least the lcdRepaint function.
 * The reason being is that this function ensures that the
 * LCD is at the very least turned on and that all IO pins
 * are set to the proper direction and are set to HIGH/LOW
 * as appropriate for then using the LCD.
 *
 * The commands needed to initialise and use the LCD screen
 * can be found in the specification we used to make this
 * code as noted at the top of this file. In the possible
 * event that the specification disappears from that url it
 * is hoped that the code here will be sufficient to
 * reconstruct the protocol for other projects in future
 * that also wish to use this type of display.
 *
 */
void initLcdScreen(void)
{
	/* In our design, we use the entire port D for the
	 * data-bus to communicate with both LCD ICs as such
	 * all of port D must be configured to be outputs,
	 * in the avr architecture this is done by setting
	 * 1 for output at the appropriate pin in the DDRD
	 * register - many other architecture use 0 for this
	 * purpose most likely because of the similarity
	 * between 0 and 'o'utput as well as 1 and 'i'nput.
	 *
	 * This should be noted as often changing the register
	 * DDRD to be some equivalent of another architecture
	 * may not be enough and may provide unexpected
	 * results.
	 *
	 */
	/*DDRD  = 0xff; (see SETUP_LCD_PINS macro) */

	/* We use pin B1 as the chip select so this must be
	 * an output, as well as B0 for the register/pixel
	 * command selecting pin.
	 */
	/*DDRB |= (1<<1) | (1<<0); (see SETUP_LCD_PINS macro) */

	/* Set direction (output) of C0 and C1 (used for selecting which IC) */
	/*DDRC |= (1<<0) | (1<<1); (see SETUP_LCD_PINS macro) */

	SETUP_LCD_PINS();

	/* When we go to upload a modification to the program there is often noise
	 * appears along the specific pins we use (as we are using the UART pins on
	 * port D), in order to keep the level this affects the LCD to a minimum
	 * we keep the data-bus pointing to the pixel memory of the ICs and not
	 * to any of the register altering commands - it should be noted that if
	 * the LCD does get a bit corrupted then there is a reset line on the
	 * LCD that works when pulled low for a couple of milliseconds.
	 */
	LCD_PIXEL_CMD();

	/* We are going to be communicating with the LCD now so
	 * we need the enable line to be low to allow this
	 */
	ENABLE_LOW();

	/* Disable both ICs */
	ICOFF();

	/*
	 * We're going to be using LCD register
	 * commands here so we set the appropriate
	 * line on the LCD to be low (we're using
	 * a macro to make things more readable though).
	 */
	LCD_REGISTER_CMD();

	/* Initialise 1st IC */
	IC1();
	lcdWrite( LCD_TURN_ONOFF_CMD(ON) );
	lcdEnableSlow();

	/* Initialise 2nd IC */
	IC2();
	lcdWrite( LCD_TURN_ONOFF_CMD(ON) );
	lcdEnableSlow();

	/* disable both ICs */
	ICOFF();

	/* done talking so we can disable the enable line */
	ENABLE_HIGH();

	/* We clear framebuffer (in Arduino ram) prior to use. */
	lcdClear();
}

void lcdClear(void)
{
	uint16_t i;

	for(i=0; i<1024; i++){
		framebuffer[i] = 0x00;
	}
}

void lcdRepaint(void)
{
	uint8_t i, j;
	volatile unsigned char* framePtr;

	/* Talking to the first half of the screen
	 * and so we only need to talk to the first
	 * IC on the LCD.
	 */
	IC1();

	for(j=0; j<8; j++){
		/* First, before we can blitz any pixels, we need
		 * to make sure to set up the right (x, y) coordinates,
		 * as such we will be changing registers in the ICs
		 * and will need to make sure the altering-register
		 * line is low */
		LCD_REGISTER_CMD();

		/* We begin by going to the appropriate line, this is
		 * not in pixels. The memory in the LCD is arranged in
		 * such a way that between both ICs each has 8 pages
		 * where each page is 64 bytes long. It is arranged in
		 * such a way that every page corresponds to a row
		 * of 8 pixels - essentially there are 8 lines and in
		 * order to write a single pixel to a coordinate given in
		 * pixels we must find the page to which those coordinates
		 * belong and go to the appropriate location in that page
		 * and set a 1 - or rather 'or' the value that was previously
		 * there with a new (1 << k) where k sets the pixel in
		 * the correct place between the 0 and 7th pixel of that row. */
		lcdWrite( LCD_GOTO_ROW(j) );
		lcdEnable();

		/* We have to get back to the start of the page now. */
		lcdWrite( LCD_GOTO_ORG()  );
		lcdEnable();

		/* Since we want to blitz out pixels (8-bits at a time) we
		 * are going to need to stop talking to the registers on the IC
		 * and hence we take the register line high (as done by our
		 * macro) */
		LCD_PIXEL_CMD();

		/* For convenience we use a pointer to the appropriate position
		 * in our framebuffer and blitz from here
		 */
		framePtr = &framebuffer[128*j];

		/* We output an entire page (64x8 bits) at a time */
		for(i=0; i<64; i++){
			lcdWrite(*framePtr++);
			lcdEnable();
		}
	}


	/* The following is the same procedures as above, however we
	 * are talking to the second IC on the LCD and hence the second
	 * half of the screen.
	 */
	IC2();

	for(j=0; j<8; j++){
		LCD_REGISTER_CMD();

		lcdWrite( LCD_GOTO_ROW(j) );
		lcdEnable();

		lcdWrite( LCD_GOTO_ORG() );
		lcdEnable();

		LCD_PIXEL_CMD();

		framePtr = &framebuffer[128*j + 64];

		for(i=0; i<64; i++){
			lcdWrite(*framePtr++);
			lcdEnable();
		}
	}
}

/*
 * As with many things, we need only to have a simple function
 * in order to derive much more, in the case of graphics, we need
 * only a single pixel plotting function from whence we can get
 * any sort of lines, circles and other graphics that we desire
 * from.
 *
 * This is our simple pixel rastering function. The screen is
 * limited to a resolution of 128x64 as such we can only address
 * coordinates from (0, 0) to (63, 63), any coordinates not in
 * that range are discarded. We use a framebuffer of 1024 bytes
 * which is enough to fit 1-bit per pixel (1024x8 = 64x2x8 i.e.
 * the entire LCD pixel memory, including both ICs).
 *
 * The framebuffer is built in such a way that it is easily
 * blitzed onto the LCD - i.e. we build it in the same page
 * by 128 method.
 */
void lcdDrawPixel(uint8_t x, uint8_t y)
{
	if(x > 127 || y > 63){
		return;
	}

	/*
	 * Because of the way we set up the LCD
	 * on the breadboard the actual image is
	 * upside down, rather than fiddle about
	 * with all the connections and turn it
	 * right-side-up, it is much easier to
	 * just flip the y-coordinate here.
	 */
	y = 63 - y;

	/*
	 * As mentioned above, the frame buffer is
	 * 1024 bytes long, each byte of this represents
	 * 8 pixels as such we must work out which
	 * byte to change, and then which bit within
	 * that byte.
	 *
	 * The y coordinate will run from 0 to 63,
	 * each 128 bytes of the framebuffer represents
	 * a page and hence there are 8 pages, each page is
	 * representative of a y coordinate from
	 *
	 *  page 0: y = 0..7
	 *  page 1: y = 8..15
	 *       .       .
	 *       .       .
	 *  page 7: y = 120..127.
	 *
	 * To find which page we are in we simply
	 * divide by 8 and ignore the remainder - or
	 * shift y by 3 to the logical right, we then times
	 * the page number by the number of bytes
	 * per page 128, or shift y by 7 to the
	 * logical left, finally we add our offset of
	 * which byte within those 128 in this page
	 * we should look at, which is representative
	 * of the x-coordinate.
	 *
	 * After we have found which byte we are wanting
	 * to modify in the framebuffer, we or this
	 * value with the appropriate 0..7 value that will
	 * place the pixel at the right coordinate.
	 * This is simply the bit given by the remainder
	 * that we ignored to get the page number.
	 *
	 */
	framebuffer[ ((y>>3)<<7) + x ] |= 1 << (y & 0x07);
}

/*
 * Since in this LCD there are no preinserted alpha-numeric
 * bitmaps from which to draw out text characters we have
 * had to implement a simple function for doing this. It would
 * have been prefer to vertically-align the bitmaps for
 * optmising the blitz into the LCD but it is more clear this
 * way - especially considering we compiled the bitmaps on upside
 * down and need to write 'backwards' into the frame buffer to
 * correct this mistake.
 *
 * Note that the lcdPrintText function uses line offsets as
 * opposed to pixels, the lines are just the pages as discussed
 * with the lcdDrawPixel function. We realise that this will
 * also automatically destroy anything in the frame buffer
 * at these points - this is a very simple text printing
 * function.
 *
 */
void lcdPrintText(char* text, uint8_t line)
{
	volatile unsigned char* framePtr;
	uint8_t  i;
	uint16_t j; /* text probably will never exceed 255 characters in length but just in case... */

	uint16_t length;

	if(line > 7){
		return;
	}

	length = strlen(text);

	if(length > 16){
		return;
	}

	for(j=0; j<length; j++){
		framePtr = &framebuffer[1023 - (line<<7) - (j*TEXT_WIDTH)];

		for(i=0; i<8; i++){
			if('A' <= text[j] && text[j] <= 'Z'){
				/* As mentioned above, in order to account for the error
				 * of compiling the bitmaps the wrong way we have to
				 * write to the framebuffer backwards
				 */
				*framePtr-- = pgm_read_byte(GET_TEXT_BYTE_UPPER(text[j]) + i);
			}else if('a' <= text[j] && text[j] <= 'z'){
				*framePtr-- = pgm_read_byte(GET_TEXT_BYTE_LOWER(text[j]) + i);
			}else if(text[j] == '.'){
				*framePtr-- = pgm_read_byte(GET_TEXT_DOT_ADDRESS         + i);
			}else{
				*framePtr-- = 0x00; /* blank space otherwise */
			}
		}
	}
}
