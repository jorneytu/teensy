#  LCD Demo Makefile, PJRC 8051 Development Board (Rev 4)

#  for an example Makefile with comments to explain the function
#  of the various parts of a Makefile, see the LED Blink example:
#    http://www.pjrc.com/tech/8051/board4/blink_sdcc.html


SDCCCFLAGS = --model-small

#ASLINKFLAGS = --code-loc 0x2000 --data-loc 0x30 --xram-loc 0x6000
ASLINKFLAGS = --code-loc 0x8000 --data-loc 0x30 --xram-loc 0x6000

%.rel : %.c
	sdcc $(SDCCCFLAGS) -c $<

all: main.hex


main.hex: main.c lcd_buffer.rel lcd_driver.rel paulmon2.rel \
  font_timBI24.rel font_9x15.rel font_5x7.rel font_lubI19.rel \
  font_7x13B.rel font_6x10.rel font_4x6.rel font_luBS14.rel
	sdcc $(SDCCCFLAGS) $(ASLINKFLAGS) $^
	packihx main.ihx > main.hex


clean:
	rm -f *.asm *.hex *.ihx *.lnk *.lst *.map *.rel *.rst *.sym *.mem



