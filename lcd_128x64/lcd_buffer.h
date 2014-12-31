#ifndef __LCD_BUFFER_INCLUDED
#define __LCD_BUFFER_INCLUDED


// Enables buf_line() to draw diagonal lines.  Commenting this
// out saves about 250 bytes.  Some of that savings is the C
// library integer divide, which you won't save if you ever
// divide ints anywhere else.  But if you only ever draw
// horizontal and vertical lines, there's no reason to leave
// this on (attempts to draw diagonal will return safely, only
// without changing anything in the buffer).
#define SUPPORT_DIAGONAL_LINES

// Include and use an index table with every font.  This
// allows buf_putchar() to quickly find the character shape.
// The cost is 2 extra bytes stored for every character in
// every font.  If you have a lot of fonts and/or need to
// save code space, and you're willing to burn some CPU time
// in buf_putchar(), just comment this out.  Be sure to
// recompile all font files (which must include this header),
// as buf_purchar() only works if the fonts are as it expects.
#define FAST_FONT_INDEX

// Use the fast (and small) assembly version.  The assembly
// code does exactly the same operations, only faster, so
// normally you'd want to use it.
#define LCD_USE_FAST_ASM


// Clear the buffer
extern void buf_clear(void);

// Set the position where text & shapes will be drawn
#define buf_set_position(x, y) (buf_x = (x), buf_y = (y))
extern data unsigned char buf_x, buf_y;

// Set the font
#define buf_set_font(f) (buf_font = (f))
extern code unsigned char * data buf_font;

// Print a character.  Normally this is called from putchar()
// and putchar() is called from printf(), printf_fast(), etc.
// So this lets you use printf to write directly to the LCD....
extern void buf_putchar(unsigned char c);

// Draw a shape to the buffer.  X position (buf_x) is incremented
extern void buf_draw(code unsigned char *shape);

// Turn on a single pixel
#define buf_pixel(x, y) buf_pixel_i((x)|((y)<<8))
extern void buf_pixel_i(unsigned int x_and_y);

// Draw a line between two points
#define buf_line(x1, y1, x2, y2) buf_line_d((x1)|((y1)<<8)\
 |((unsigned long)(x2)<<16)|((unsigned long)(y2)<<24))
extern void buf_line_d(unsigned long two_pts);

// Normally you will call the functions (or macros) above to
// draw on the buffer.  However, you can also write directly
// to it if you like!
extern xdata unsigned char lcd_buffer[1024];

#endif
