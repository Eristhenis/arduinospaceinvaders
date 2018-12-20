#ifndef lcdh
#define lcdh

#define UPPER_CASE_ALPHA_LENGTH 26
#define LOWER_CASE_ALPHA_LENGTH 26

#define UPPER_CASE_ADDRESS      (TEXT + 0)
#define LOWER_CASE_ADDRESS      (TEXT + (UPPER_CASE_ALPHA_LENGTH*8))

#define BYTES_PER_CHARACTER     8
#define TEXT_WIDTH              8

#define GET_TEXT_BYTE_UPPER(x)  (UPPER_CASE_ADDRESS + (x-'A')*BYTES_PER_CHARACTER)
#define GET_TEXT_BYTE_LOWER(x)  (LOWER_CASE_ADDRESS + (x-'a')*BYTES_PER_CHARACTER)
#define GET_TEXT_DOT_ADDRESS    (TEXT + 52*8)

void initLcdScreen(void);
void lcdRepaint(void);
void lcdClear(void);
void lcdDrawPixel(uint8_t x, uint8_t y);
void lcdPrintText(char* text, uint8_t line); /* note this function is line based and not pixel based */

#endif
