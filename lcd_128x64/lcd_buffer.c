#include "lcd_buffer.h"
//#include <stdio.h>
//#include "paulmon2.h"
//#define PHEX 0x0034
//#define NEWLINE 0x0048


data unsigned char buf_x, buf_y;
code unsigned char * data buf_font;
xdata unsigned char lcd_buffer[1024];


static bit down;
static code unsigned char bitmask[8]={1,2,4,8,16,32,64,128};



#ifndef LCD_USE_FAST_ASM

// This C code is intended to be readable, so you can understand
// how these functions work, and make changes without having to
// dig through the heavily optimized assembly version below.


// buf_clear - clear every pixel in the buffer
//
void buf_clear(void)
{
	xdata unsigned char *p;
	unsigned char i=0;

	p = lcd_buffer;
	do {
		*p++ = 0;
		*p++ = 0;
		*p++ = 0;
		*p++ = 0;
		i--;
	} while (i);
}


// buf_putchar - draw a character to the LCD buffer.  The position
// 	is set by buf_x and buf_y.  The font used is controlled by
// 	buf_font.  The font data is just a list of shape data, so
// 	all this function does is find the address of the shape
// 	and then call buf_draw.
//
void buf_putchar(unsigned char c)
{
	unsigned char first, num;
	code unsigned char *font;

	font = buf_font;
	first = *font++;
	if (c < first) return;
	c -= first;
	num = *font++;
	if (c >= num) return;
#ifdef FAST_FONT_INDEX
	buf_draw(buf_font + (font[c * 2] | (font[c * 2 + 1] << 8)));
#else
	while (c) {
		num = *font++;
		font++;
		if (num & 0x80) font++;
		font += ((num & 0x1F) + 1) * (((num & 0x60) >> 5) + 1);
		c--;
	}
	buf_draw(font);
#endif
}


// buf_draw - draw a shape to the LCD buffer.  Shapes may be up
// to 32 by 32 pixels.  After drawing, buf_x is advanced automatically.
// buf_y is not changed, even if the shape is only drawn partially
// due to the right edge of the screen.
//
void buf_draw(code unsigned char *shape)
{
	xdata unsigned char *buf;
	unsigned char width, height, y_offset, tmp;
	signed char lead, trail;
	unsigned char y_abs, y_shift, i;

	// unpack the 2 or 3 byte character size info
	tmp = *shape++;
	width = (tmp & 0x1F) + 1;
	height = ((tmp >> 5) & 0x3) + 1;
	if (tmp & 0x80) {
		y_offset = *shape++;
		tmp = *shape++;
		lead = (tmp >> 4) - 4;
		trail = (tmp & 15) - 5;
	} else {
		tmp = *shape++;
		y_offset = (tmp >> 5);
		tmp &= 0x1F;
		lead = (tmp / 6) - 1;
		trail = (tmp % 6) - 2;
	}
	// font info:  (2 or 3 bytes)
	//	width (in pixels)	1 to 32		5 bits
	//	height (in bytes)	1 to 4		2 bits
	//	offset format		short or long	1 bit
	//
	// short offset format
	//	y_offset (pixels)	0 to 7		3 bits
	//	leading space		-1 to 3	 	2.33 bits
	//	trailing space		-2 to 3		2.6 bits
	//
	// long offset format
	//	y_offset (pixels)	0 to 31		5 bits
	//	leading space		-4 to 11	4 bits
	//	trailing space		-5 to 10	4 bits

	//apply the x leading space
	if (lead >= 0 || buf_x >= -lead) buf_x += lead;
	if (buf_x > 127) buf_x = 127;

	// y position for top of character
	y_abs = (buf_y + y_offset) >> 3;
	if (y_abs > 7) return;
	y_shift = (buf_y + y_offset) & 7;

	// copy the pixel data into the buffer, shifting as necessary
	do {
		buf = lcd_buffer + buf_x + y_abs * 128;
		for (i=0; i<width; i++) {
			if (buf_x <= 127) {
				*buf |= (*shape << y_shift);
				if (y_abs < 7) {
					*(buf + 128) |= (*shape >> (8 - y_shift));
				}
				buf++;
			}
			buf_x++;
			shape++;
		}
		if (--height == 0) break;
		if (++y_abs > 7) break;
		buf_x -= width;
	} while (1);

	// apply trailing space
	buf_x += trail;
	if (buf_x > 127) buf_x = 127;
}


// buf_pixel - turn on a single pixel in the LCD buffer.  This is
// intended to be called with a macro combines both X and Y
// coordinates into a single 16 bit integer.  Normally, you would
// call this as:
//
//	buf_pixel(x, y);
//
void buf_pixel_i(unsigned int x_and_y)
{
	unsigned char x, y;

	x = x_and_y & 127;
	y = x_and_y >> 8;
	*(lcd_buffer + x + ((y & 0x38) << 4)) |= bitmask[y & 7];
}


// buf_line - draw a line between two points.  This is intended to
// called with a macro that combines all 4 coordinates into a 32 bit
// long.  Normally, you would call this as:
//
//	buf_line(x1, y1, x2, y2);
//
void buf_line_d(unsigned long two_pts)
{
	unsigned char x1, y1, x2, y2, tmp;
	xdata unsigned char *p;
	unsigned int slope, x_accum;

	x1 = two_pts & 127;
	y1 = (two_pts >> 8) & 63;
	x2 = (two_pts >> 16) & 127;
	y2 = (two_pts >> 24) & 63;

	if (x1 == x2) {
		// vertical line
		if (y1 > y2) {
			tmp = y1; y1 = y2; y2 = tmp;
		}
		tmp = bitmask[y1 & 7];
		p = lcd_buffer + x1 + ((y1 & 0x38) << 4);
		y1 = y2 - y1 + 1;
		do {
			*p |= tmp;
			tmp = ((tmp >> 7) | (tmp << 1));
			if (tmp & 1) p += 128;
		} while (--y1);
		return;
	}
	if (x1 > x2) {
		tmp = y1; y1 = y2; y2 = tmp;
		tmp = x1; x1 = x2; x2 = tmp;
	}
	if (y2 == y1) {
		// horizontal line
		tmp = bitmask[y1 & 7];
		p = lcd_buffer + x1 + ((y1 & 0x38) << 4);
		x1 = x2 - x1 + 1;
		do {
			*p++ |= tmp;
		} while (--x1);
		return;
	}
#ifdef SUPPORT_DIAGONAL_LINES
	  else if (y2 > y1) {
		// diagonal line, sloping downward
		down = 1;
		tmp = y2 - y1;
	} else {
		// diagonal line, sloping upward
		down = 0;
		tmp = y1 - y2;
	}
	slope = ((x2 - x1) << 8) / tmp;
	tmp = bitmask[y1 & 7];
	p = lcd_buffer + x1 + ((y1 & 0x38) << 4);
	if (slope >= 256) {
		x_accum = slope / 2;
		x1 = x2 - x1 + 1;
		do {
			*p++ |= tmp;
			x_accum += 256;
			if (x_accum >= slope) {
				x_accum -= slope;
				if (down) {
					tmp = ((tmp >> 7) | (tmp << 1));
					if (tmp & 1) p += 128;
				} else {
					tmp = ((tmp << 7) | (tmp >> 1));
					if (tmp & 128) p -= 128;
				}
			}
		} while (--x1);
	} else {
		x_accum = 128;
		y1 = y2 - y1 + 1;
		do {
			*p |= tmp;
			x_accum += slope;
			if (x_accum >= 256) {
				p++;
				x_accum -= 256;
			}
			if (down) {
				tmp = ((tmp >> 7) | (tmp << 1));
				if (tmp & 1) p += 128;
			} else {
				tmp = ((tmp << 7) | (tmp >> 1));
				if (tmp & 128) p -= 128;
			}
		} while (--y1);
	}
#endif // SUPPORT_DIAGONAL_LINES
}



#else // LCD_USE_FAST_ASM


// This assembly code has been optimized for speed.  Careful attention
// has been paid to minimizing time in loops.  The size is about 500
// bytes less than the compiled C code (SDCC version 2.7.0, small model).
//
// With only a couple small exceptions, which are well commented, this
// assembly code follows the algorithm and form of the C code above.
// To read this assembly code, it's helpful to place a copy of the C
// code side by side with the assembly.
//

void buf_clear(void)
{
	_asm
	mov	dptr, #_lcd_buffer
	clr	a
	mov	b, a
buf_clear_loop:
	movx	@dptr, a
	inc	dptr
	movx	@dptr, a
	inc	dptr
	movx	@dptr, a
	inc	dptr
	movx	@dptr, a
	inc	dptr
	djnz	b, buf_clear_loop
	_endasm;
}


void buf_putchar(unsigned char c)
{
	c;
	_asm
	mov	r0, dpl

	//mov	dptr, #_buf_font
	//movx	a, @dptr
	//mov	r1, a
	//inc	dptr
	//movx	a, @dptr
	//mov	dph, a
	//mov	r2, a
	//mov	dpl, r1


	mov	dpl, _buf_font
	mov	dph, (_buf_font + 1)
	clr	a
	movc	a, @a+dptr
	xch	a, r0
	clr	c
	subb	a, r0
	jc	buf_putchar_end
	mov	r0, a
	mov	a, #1
	inc	dptr
	clr	a
	movc	a, @a+dptr
	mov	r3, a
	clr	c
	mov	a, r0
	subb	a, r3
	jnc	buf_putchar_end
	inc	dptr
#ifdef FAST_FONT_INDEX
	mov	a, dpl
	add	a, r0
	jnc	001$
	inc	dph
001$:	add	a, r0
	jnc	002$
	inc	dph
002$:	mov	dpl, a
	clr	a
	movc	a, @a+dptr
	mov	r0, a
	mov	a, #1
	movc	a, @a+dptr
	xch	a, r0
	add	a, _buf_font
	mov	dpl, a
	mov	a, r0
	addc	a, (_buf_font + 1)
	mov	dph, a
	ljmp	_buf_draw
#else
	mov	a, r0
	jz	buf_putchar_found
buf_putchar_loop:
	clr	a
	movc	a, @a+dptr
	mov	r1, #2
	jnb	acc.7, 001$
	inc	r1
001$:	mov	b, a
	anl	a, #0x60
	swap	a
	rr	a
	inc	a
	xch	a, b
	anl	a, #0x1F
	inc	a
	mul	ab
	add	a, r1
	add	a, dpl
	mov	dpl, a
	clr	a
	addc	a, dph
	mov	dph, a
	djnz	r0, buf_putchar_loop
buf_putchar_found:
	ljmp	_buf_draw
#endif
buf_putchar_end:
	_endasm;
}


void buf_draw(code unsigned char *shape)
{
	shape;
	_asm
	clr	a
	movc	a, @a+dptr
	inc	dptr
	mov	c, acc.7
	mov	r0, a
	anl	a, #0x1F
	inc	a
	xch	a, r0			// r0 = width
	swap	a
	rr	a
	anl	a, #0x03
	inc	a
	mov	r1, a			// r1 = height
	jnc	buf_draw_2byte_info
buf_draw_3byte_info:
	clr	a
	movc	a, @a+dptr
	inc	dptr
	mov	r2, a			// r2 = y_offset
	clr	a
	movc	a, @a+dptr
	inc	dptr
	mov	r3, a
	swap	a
	anl	a, #15
	add	a, #256 - 4
	xch	a, r3			// r3 = lead
	anl	a, #15
	add	a, #256 - 5
	push	acc			// trail pushed onto stack
	sjmp	buf_draw_x_lead
buf_draw_2byte_info:
	clr	a
	movc	a, @a+dptr
	inc	dptr
	mov	r2, a
	anl	a, #0xE0
	swap	a
	rr	a
	xch	a, r2			// r2 = y_offset
	anl	a, #0x1F
	mov	b, #6
	div	ab
	dec	a
	mov	r3, a			// r3 = lead
	mov	a, b
	add	a, #256 - 2
	push	acc			// trail pushed onto stack
buf_draw_x_lead:
	mov	a, r3
	jnb	acc.7, buf_draw_apply_lead
	add	a, _buf_x
	jc	buf_draw_apply_lead
	mov	a, _buf_x
	sjmp	buf_draw_x_check
buf_draw_apply_lead:
	mov	a, _buf_x
	add	a, r3
buf_draw_x_check:
	jnb	acc.7, 001$
	mov	acc, #127
001$:	mov	_buf_x, a
buf_draw_ycalc:
	mov	a, _buf_y
	add	a, r2
	swap	a
	rl	a
	anl	a, #0x1F
	mov	r3, a			// r3 = y_abs
	anl	a, #0xF8
	jz	buf_draw_ycalc_2
	pop	acc			// if (y_abs > 7) return
	ret
buf_draw_ycalc_2:
	mov	a, _buf_y
	add	a, r2
	anl	a, #0x07
	mov	r2, a
	// this is a place where the assembly code diverges somewhat
	// from the C code above.  Rather than only storing y_shift,
	// which requires only 3 bits of data, we also compute
	// (8 - y_shift) here and store it in the upper 4 bits of
	// the same variable.
	cpl	a
	add	a, #9
	swap	a
	orl	a, r2
	mov	b, a			// b = y_shift and (8 - y_shift)
buf_draw_y_loop:
	mov	a, r3
	rr	a
	anl	a, #128
	add	a, _buf_x
	add	a, #_lcd_buffer
	mov	r4, a			// r5/r4 = buf pointer
	mov	a, r3
	rr	a
	anl	a, #127
	addc	a, #_lcd_buffer >> 8
	mov	r5, a
	mov	r2, ar0			// r2 = width loop count
buf_draw_x_loop:
	mov	a, _buf_x
	jb	acc.7, buf_draw_x_next	// if (buf_x <= 127)
	clr	a
	movc	a, @a+dptr
	mov	r6, a			// r6 = *shape
	push	dpl
	push	dph
	mov	dpl, r4
	mov	dph, r5			// now dptr is buf pointer
	// the simplest way (and the way SDCC uses) to shift by a
	// variable number of bits is with a simple loop.  But that
	// is slow.  Instead, we'll just look at the 3 bits (because
	// we know from the calc above the shift is always 0 to 7),
	// and do 4, 2, and 1 bits of shift for each one if it's set.
	jnb	b.2, 002$
	swap	a
	anl	a, #0xF0
002$:	jnb	b.1, 003$
	rl	a
	rl	a
	anl	a, #0xFC
003$:	jnb	b.0, 004$
	rl	a
	anl	a, #0xFE
004$:	mov	r7, a			// r7 = (*shape << y_shift)
	movx	a, @dptr
	orl	a, r7			// *buf |= r7
	movx	@dptr, a
buf_draw_x_2nd_row:
	// this is another tiny change from the C code above.  We
	// know from the calc above that (8 - y_shift) will always
	// be in the range of 1 to 8.  Testing bit 7 checks for the
	// case of 8, which will result in the shift producing a
	// zero, which isn't going to do anything when OR'd with the
	// buffer memory.  So if it's 8, we'll just skip all this
	// work.
	jb	b.7, buf_draw_x_skip_2nd_row
	mov	a, r3
	add	a, #256 - 7		// if (y_abs < 7)
	jc	buf_draw_x_skip_2nd_row
	mov	a, r4
	add	a, #128			// buf + 128
	mov	dpl, a
	clr	a
	addc	a, r5
	mov	dph, a
	// again, a shift operation by conditionally shifting based
	// on the bits, rather than a loop.  Fortunately, the calc
	// above stored all the bits for both these shifts in the B
	// register, which is bit addressable.
	mov	a, r6
	jnb	b.6, 005$
	swap	a
	anl	a, #0x0F
005$:	jnb	b.5, 006$
	rr	a
	rr	a
	anl	a, #0x3F
006$:	jnb	b.4, 007$
	rr	a
	anl	a, #0x7F
007$:	mov	r7, a			// r7 = (*shape >> (8 - y_shift))
	movx	a, @dptr
	orl	a, r7			// *(buf + 128) |= r7
	movx	@dptr, a
	mov	dpl, r4
	mov	dph, r5
buf_draw_x_skip_2nd_row:
	inc	dptr			// buf++
	mov	r4, dpl
	mov	r5, dph
	pop	dph
	pop	dpl
buf_draw_x_next:
	inc	_buf_x			// buf_x++
	inc	dptr			// shape++
	djnz	r2, buf_draw_x_loop
	dec	r1			// --height
	mov	a, r1
	jz	buf_draw_trail
	inc	r3			// ++y_abs
	mov	a, r3
	anl	a, #0xF8
	jnz	buf_draw_trail
	mov	a, _buf_x
	clr	c
	subb	a, r0			// buf_x -= width
	mov	_buf_x, a
	ljmp	buf_draw_y_loop
buf_draw_trail:
	pop	acc
	add	a, _buf_x		// buf_x += trail
	jnb	acc.7, 010$
	mov	a, #127
010$:	mov	_buf_x, a
	_endasm;
}



void buf_pixel_i(unsigned int x_and_y)
{
	x_and_y;
	_asm
	mov	a, dpl
	anl	a, #127
	mov	r2, a		// r2 is x
	mov	a, dph
	mov	r3, a		// r3 is y
	lcall	line_calc_ptr_and_bitmask
	movx	a, @dptr
	orl	a, r4
	movx	@dptr, a
	_endasm;
}




void buf_line_d(unsigned long two_pts)
{
	two_pts;
	_asm
	mov	r2, dpl
	anl	ar2, #127
	mov	r3, dph
	anl	ar3, #63
	anl	a, #63
	xch	a, b		// r2 = x1 , r3 = y1, acc = x2, b = y2
	anl	a, #127
	cjne	a, ar2, line_x_ne
line_vertical:
	clr	c
	mov	a, b
	subb	a, r3		//  if (y1 > y2) {
	jnc	line_vertical_2
	mov	a, r3
	mov	r3, b		// tmp = y1; y1 = y2; y2 = tmp;
	mov	b, a
line_vertical_2:
	lcall	line_calc_ptr_and_bitmask
	mov	a, b
	clr	c
	subb	a, r3
	inc	a
	mov	r3, a
line_vertical_loop:
	movx	a, @dptr
	orl	a, r4
	movx	@dptr, a
	mov	a, r4
	rl	a
	mov	r4, a
	jnb	acc.0, line_vertical_next
	lcall	dptr_inc_128
line_vertical_next:
	djnz	r3, line_vertical_loop
	ret
line_calc_ptr_and_bitmask:
	mov	a, r3
	anl	a, #7
	mov	dptr, #_bitmask
	movc	a, @a+dptr
	mov	r4, a		// r4 is bitmask to OR
	mov	a, r3
	swap	a
	anl	a, #128
	add	a, r2
	add	a, #_lcd_buffer
	mov	dpl, a
	mov	a, r3
	swap	a
	anl	a, #3
	addc	a, #(_lcd_buffer >> 8)
	mov	dph, a
	ret
dptr_inc_128:
	mov	a, dpl
	add	a, #128
	mov	dpl, a
	clr	a
	addc	a, dph
	mov	dph, a
	ret
dptr_dec_128:
	mov	a, dpl
	add	a, #0x80
	mov	dpl, a
	mov	a, dph
	addc	a, #0xFF
	mov	dph, a
	ret
line_x_ne:			// r2 = x1 , r3 = y1, acc = x2, b = y2
	jnc	line_x_ok
	xch	a, r2
	push	ar3
	mov	r3, b
	pop	b
line_x_ok:
	xch	a, b		// r2 = x1 , r3 = y1, b = x2, acc = y2
	cjne	a, ar3, line_diag
line_horiz:
	lcall	line_calc_ptr_and_bitmask
	mov	a, b
	clr	c
	subb	a, r2
	inc	a
	mov	r2, a
line_horiz_loop:
	movx	a, @dptr
	orl	a, r4
	movx	@dptr, a
	inc	dptr
	djnz	r2, line_horiz_loop
	ret
line_diag:
#ifdef SUPPORT_DIAGONAL_LINES
	push	acc		// push y2 onto stack 1st (we need acc now)
	jc	line_diag_upward
line_diag_downward:
	setb	_down
	clr	c
	subb	a, r3
	sjmp	line_slope
line_diag_upward:
	clr	_down
	clr	c
	subb	a, r3
	cpl	a
	inc	a
line_slope:
#if defined(SDCC_PARMS_IN_BANK1)
	mov	b1_0, a
	clr	a
	mov	b1_1, a
#elif defined(SDCC_STACK_AUTO)	//  or --int-long-reent (TODO: compiler define?)
 #if !defined(SDCC_USE_XSTACK)
	push	acc
	clr	a
	push	acc
 #else
	mov	r0, _spx
	movx	@r0, a
	inc	r0
	clr	a
	movx	@r0, a
	inc	r0
	mov	_spx, r0
 #endif
#elif defined(SDCC_MODEL_SMALL)
	mov	__divsint_PARM_2, a
	clr	a
	mov	(__divsint_PARM_2 + 1), a
#elif defined(SDCC_MODEL_MEDIUM)
	mov	r0, #__divsint_PARM_2
	movx	@r0, a
	inc	r0
	clr	a
	movx	@r0, a
#elif defined(SDCC_MODEL_LARGE)
	mov	dptr, #__divsint_PARM_2
	movx	@dptr, a
	inc	dptr
	clr	a
	movx	@dptr, a
#elif
#error "Unknown memory model, contact paul@pjrc.com"
#endif
	mov	dpl, a
	mov	a, b
	clr	c
	subb	a, r2 
	mov	dph, a
	push	b		// push x2 onto stack (and leave it there a while)
	push	ar2
	push	ar3
	lcall	__divsint
	mov	r0, dpl		// r1/r0 = slope
	mov	r1, dph
	pop	ar3
	pop	ar2		// r2 = x1 , r3 = y1   x2 and y2 still on stack
	lcall	line_calc_ptr_and_bitmask
	mov	a, r1
	jz	line_accute
line_shallow:
	clr	c
	rrc	a
	mov	r7, a		// r7/r6 = x_accum
	mov	a, r0
	rrc	a
	mov	r6, a
	pop	acc
	clr	c
	subb	a, r2
	inc	a
	mov	r2, a
	dec	sp
line_shallow_loop:
	movx	a, @dptr
	orl	a, r4
	movx	@dptr, a
	inc	dptr
	inc	r7
	clr	c
	mov	a, r6
	subb	a, r0
	mov	r3, a
	mov	a, r7
	subb	a, r1
	jc	line_shallow_next
	mov	r7, a
	mov	r6, ar3
	jnb	_down, line_shallow_upward
line_shallow_downward:
	mov	a, r4
	rl	a
	mov	r4, a
	jnb	acc.0, line_shallow_next
	lcall	dptr_inc_128
	sjmp	line_shallow_next
line_shallow_upward:
	mov	a, r4
	rr	a
	mov	r4, a
	jnb	acc.7, line_shallow_next
	lcall	dptr_dec_128
line_shallow_next:
	djnz	r2, line_shallow_loop
	ret
line_accute:
	mov	r6, #128
	mov	r7, a
	dec	sp
	pop	acc
	clr	c
	subb	a, r3
	inc	a
	mov	r3, a
line_accute_loop:
	movx	a, @dptr
	orl	a, r4
	movx	@dptr, a
	mov	a, r6
	add	a, r0
	mov	r6, a
	mov	a, r7
	addc	a, r1
	mov	r7, a
	jz	line_accute_skip_xinc
	inc	dptr
	dec	r7
line_accute_skip_xinc:
	jnb	_down, line_accute_upward
line_accute_downward:
	mov	a, r4
	rl	a
	mov	r4, a
	jnb	acc.0, line_accute_next
	lcall	dptr_inc_128
	sjmp	line_accute_next
line_accute_upward:
	mov	a, r4
	rr	a
	mov	r4, a
	jnb	acc.7, line_accute_next
	lcall	dptr_dec_128
line_accute_next:
	djnz	r3, line_accute_loop
#endif // SUPPORT_DIAGONAL_LINES
	_endasm;
}


#endif	// LCD_USE_FAST_ASM



