#ifndef __LCD_DRIVER_INCLUDED
#define __LCD_DRIVER_INCLUDED


#pragma callee_saves lcd_putchar,lcd_init,lcd_set_xy_raw,lcd_clear,lcd_home

extern void lcd_update(void);
extern void lcd_init(void);
extern void lcd_set_xy_raw(unsigned int x_and_y);


/* This macro defines the lcd_set_xy function with two parameters
 * that get merged into a single 16 bit parameter when calling
 * the lcd_set_xy_raw code.  The idea is to allow the common case
 * of two constants to turn into a single MOV DPTR, #XXX instruction
 * and avoid SDCC's overhead of passing a second parameter.
 */
#define lcd_set_xy(x, y) (lcd_set_xy_raw((x) + ((y) << 8)))


/* to use a different size display, change this (untested on 4 lines) */

#define LCD_HORIZ_SIZE  20
//#define LCD_HORIZ_SIZE  16
#define LCD_VERT_SIZE   2
#define LCD_BYTES_PER_LINE  (128 / LCD_VERT_SIZE)

#endif
