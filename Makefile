# Compiler definitions
CROSS_COMPILE ?= avr-
CC = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
SIZE = $(CROSS_COMPILE)size
MCU ?= attiny85
CFLAGS ?= -g -mmcu=$(MCU) -O3 -Werror -Woverflow
CPPFLAGS ?= -DF_CPU=$(FREQ) -D_POLY_CFG=\"poly_cfg.h\"
LDFLAGS ?= -mmcu=$(MCU) -O3 -Wl,--as-needed
PROG ?= avrdude
PROG_ARGS ?= -B 10 -c stk500v2 -P /dev/ttyACM0
PROG_DEV ?= t85

BINDIR ?= bin
SRCDIR ?= .
OBJDIR ?= obj

LFUSE=0xc1
HFUSE=0xdf
EFUSE=0xff
FREQ=16000000

.PHONY: setfuse all clean

-include local.mk

all: bindir/synth.hex

%.hex: %.elf
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

$(BINDIR)/synth.elf: $(OBJDIR)/main.o $(OBJDIR)/poly.a
	@[ -d $(BINDIR) ] || mkdir $(BINDIR)
	$(CC) -g -o $@ $(LDFLAGS) $^

$(OBJDIR)/poly.a: $(OBJDIR)/adsr.o $(OBJDIR)/waveform.o
	$(AR) rcs $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@[ -d $(OBJDIR) ] || mkdir $(OBJDIR)
	$(CC) -o $@ -c $(CPPFLAGS) $(CFLAGS) $<

clean:
	-rm -fr $(OBJDIR) $(BINDIR)

%.ihex: %.elf
	$(OBJCOPY) -j .text -j .data -O ihex $^ $@

%.pgm: %.ihex
	$(PROG) $(PROG_ARGS) -p $(PROG_DEV) -U flash:w:$^:i

setfuse:
	$(PROG) $(PROG_ARGS) -p $(PROG_DEV) \
		-U lfuse:w:$(LFUSE):m \
		-U hfuse:w:$(HFUSE):m \
		-U efuse:w:$(EFUSE):m 
