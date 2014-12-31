/* 128x64 LCD Demo code
 *   http://www.pjrc.com/
 *
 */

#include "lcd_driver.h"
#include "lcd_buffer.h"
#include "lcd_fonts.h"
#include "paulmon2.h"
#include <stdio.h>	/* for printf_fast, etc */



bit print_to_lcd=0;


void main()
{
	unsigned char i;

	// initialize the LCD hardware
	// this also clears the buffer
	lcd_init();

	// calls to print_fast will print to the LCD
	print_to_lcd = 1;

	// print in a huge, fancy font
	//buf_set_font(font_lubI19);
	
	buf_set_font(font_timBI24);
	buf_set_position(6, 0);
	printf_fast("128 x 64");

	//buf_set_font(font_luBS14);
	//buf_set_font(font_6x10);
	buf_set_font(font_7x13B);
	buf_set_position(25, 30);
	printf_fast("Graphic LCD");

	// print in a small font
	//buf_set_font(font_6x10);
	//buf_set_font(font_5x7);
	
	buf_set_font(font_4x6);
	buf_set_position(0, 58);
	printf_fast("Font sizes from large to tiny...");

	// print in a bigger font
	//buf_set_font(font_9x15);
	//buf_set_font(font_6x13B);
	//buf_set_font(font_7x13B);
	//buf_set_position(6, 44);
	//printf_fast("Voltage: 4.57");

	for (i=4; i<120; i+=20) {
		buf_line(i, 42, i+20,  55);
		buf_line(i+20, 42, i+20,  55);
		//buf_line(i+9, 57, i+18, 40);  //(bug, fixme)
	}

	// draw some lines
	//buf_line(8, 10, 100, 40);
	//buf_line(8, 10, 20, 40);

	// draw a few pixels too
	//buf_pixel(120, 10);
	//buf_pixel(120, 12);
	//buf_pixel(120, 14);
	//buf_pixel(121, 16);

	// and then put it all on the display at once
	// all "drawing" is done on the buffer, which
	// is then quickly copied to the actual LCD
        lcd_update();

	print_to_lcd = 0;
	printf_fast("press any key to reboot");
	pm2_cin();
	pm2_restart();  /* ESC to quit */
}






/* printf may seem like a complicated beast, but all it really
 * does is take all the stuff you pass in and generate a bunch
 * of calls to putchar().
 * 
 * This simple but useful putchar() example uses a global variable
 * to switch where printf's output will go.  When (print_to_lcd == 1)
 * the characters will end up on the LCD, and when is it set to
 * zero, they will go to the serial port via PAULMON2's pm2_cout()
 * function.  If you have some other serial port driver, or some
 * other I/O device that printf() should be able to "print to",
 * all you need to do is adapt putchar() to route the stream of
 * characters where you want them.
 */

void putchar(char c)
{
	if (print_to_lcd) {
		buf_putchar(c);
	} else {
		pm2_cout(c);
	}
}



