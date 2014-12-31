#include "lcd_driver.h"
#include "lcd_buffer.h"



#ifndef LCD_USE_FAST_ASM


volatile xdata at 0xFE04 unsigned char left_cmd;
volatile xdata at 0xFE05 unsigned char left_status;
volatile xdata at 0xFE06 unsigned char left_data;
volatile xdata at 0xFE08 unsigned char right_cmd;
volatile xdata at 0xFE09 unsigned char right_status;
volatile xdata at 0xFE0A unsigned char right_data;

#define left_command(cmd)  {while (left_status & 0x80) ; left_cmd = (cmd); }
#define right_command(cmd) {while (right_status & 0x80) ; right_cmd = (cmd); }
#define left_write(cmd)    {while (left_status & 0x80) ; left_data = (cmd); }
#define right_write(cmd)   {while (right_status & 0x80) ; right_data = (cmd); }

// Copy the LCD buffer (in RAM) to the LCD memory.  This allows
// quick update which minimizes flicker.
//
void lcd_update(void)
{
	unsigned char x, y;
	xdata unsigned char *p;

	p = lcd_buffer;
	for (y=0; y<8; y++) {
		right_command(0xB8 | y);
		left_command(0xB8 | y);
		right_command(0x40);
		left_command(0x40);
		for (x=0; x<64; x++) {
			right_write(*(p + 64));
			left_write(*p);
			p++;
		}
		p += 64;
	}
}

void lcd_init(void)
{
	// todo: check if LCD is in reset state, wait for it to be ready
	// todo: what happens if this code is run on a board with the LCD?
	//
	right_command(0xC0);
	left_command(0xC0);		// set display start line to zero
	buf_clear();
	lcd_update();
	right_command(0x3F);
	left_command(0x3F);		// turn on the display
}


#else



void lcd_update(void)
{
	_asm
	// This code is optimized for speed.
	// TODO: measure actual speed
	// Best theoretical speed is 1us per access, plus 4.44 us
	// typical busy time, times 1056 accesses = 5.75 ms
	// y = 0
	mov	r2, #0
	// p = lcd_buffer
	mov	dptr, #_lcd_buffer
	mov	p2, #0xFE	/* select LCD when movx with R0 */
001$:	// outer loop -- for (y=0; y<8; y++) {
	// right_command(0xB8 | y);
	mov	r0, #0x09	/* right status */
002$:	movx	a, @r0
	jb	acc.7, 002$
	mov	a, #0xB8
	orl	a, r2
	dec	r0		/* right command */
	movx	@r0, a
	// left_command(0xB8 | y);
	mov	r0, #0x05	/* left status */
003$:	movx	a, @r0
	jb	acc.7, 003$
	mov	a, #0xB8
	orl	a, r2
	dec	r0		/* left command */
	movx	@r0, a
	// right_command(0x40);
	mov	r0, #0x09	/* right status */
004$:	movx	a, @r0
	jb	acc.7, 004$
	mov	a, #0x40
	dec	r0		/* right command */
	movx	@r0, a
	// left_command(0x40);
	mov	r0, #0x05	/* left status */
005$:	movx	a, @r0
	jb	acc.7, 005$
	mov	a, #0x40
	dec	r0		/* left command */
	movx	@r0, a
	// x = 0
	mov	r3, #64
006$:	// inner loop -- for (x=0; x<64; x++) {
	// right_write(*(p + 64));
	mov	r0, #0x09	/* right status */
	mov	a, dpl
	add	a, #64
	mov	dpl, a
	mov	a, dph
	addc	a, #0
	mov	dph, a
007$:	movx	a, @r0
	jb	acc.7, 007$
	movx	a, @dptr
	inc	r0		/* right data */
	movx	@r0, a
	// left_write(*p);
	mov	r0, #0x05	/* left status */
	mov	a, dpl
	add	a, #0xC0
	mov	dpl, a
	mov	a, dph
	addc	a, #0xFF
	mov	dph, a
010$:	movx	a, @r0
	jb	acc.7, 010$
	movx	a, @dptr
	inc	r0		/* left data */
	movx	@r0, a
	// p++;
	inc	dptr
	djnz	r3, 006$
	// p += 64;
	mov	a, dpl
	add	a, #64
	mov	dpl, a
	mov	a, dph
	addc	a, #0
	mov	dph, a
	// y++
	inc	r2
	cjne	r2, #8, 001$
	mov	p2, #0xFF	/* de-select LCD */
	_endasm;
}


void lcd_init(void)
{
	_asm
	mov	dptr, #0xFE09
	mov	r2, #0xC0
	lcall	001$
	mov	dptr, #0xFE05
	mov	r2, #0xC0
	lcall	001$
	lcall	_buf_clear
	lcall	_lcd_update
	mov	dptr, #0xFE09
	mov	r2, #0x3F
	lcall	001$
	mov	dptr, #0xFE05
	mov	r2, #0x3F
001$:	movx	a, @dptr
	jb	acc.7, 001$
	dec	dpl
	mov	a, r2
	movx	@dptr, a
	_endasm;
}

#endif



