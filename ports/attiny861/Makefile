CROSS_COMPILE ?= avr-

MCU ?= attiny861
CFLAGS ?= -g -mmcu=$(MCU) -O3 -Werror -Woverflow
CPPFLAGS ?= -I$(SRCDIR) -DF_CPU=$(FREQ) -DSYNTH_CFG=\"poly_cfg.h\"
LDFLAGS ?= -mmcu=$(MCU) -O3 -Wl,--as-needed
PROG ?= avrdude
PROG_ARGS ?= -B 10 -c stk500v2 -P /dev/ttyACM0
PROG_DEV ?= t861
TARGET=$(BINDIR)/synth.ihex
INCLUDES += -I$(SRCDIR) -I$(PORTDIR)
OBJECTS += $(OBJDIR)/main.o

# Low fuse:
# 	CLKDIV8=1	: 0x80	: Do not divide clock by 8
# 	CLKOUT=1	: 0x40	: Do not enable CLKOUT
# 	SUT=2		: 0x20	: Default start-up time
# 	CKSEL=1		: 0x01	: Use 16MHz PLL-derived clock
LFUSE=0xc1
# High fuse:
# 	RSTDISBL=1	: 0x80	: Enable external reset
# 	DWEN=1		: 0x40	: Disable DebugWire
# 	SPIEN=0		: 0x00	: Enable SPI programming
# 	WDTON=1		: 0x10	: Disable watchdog
# 	EESAVE=1	: 0x08	: EEPROM not preserved during erase
# 	BODLEVEL=4	: 0x06	: Brown-out detection 2.5-2.9V
HFUSE=0xdd
# Extended fuse:
# 	SELFPRGEN=unprogrammed	: Disable self programming
EFUSE=0xff
FREQ=16000000

all: $(TARGET)
program: $(BINDIR)/synth.ihex
	$(PROG) $(PROG_ARGS) -p $(PROG_DEV) -U flash:w:$^:i \
		-U lfuse:w:$(LFUSE):m \
		-U hfuse:w:$(HFUSE):m \
		-U efuse:w:$(EFUSE):m 
