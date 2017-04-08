CROSS_COMPILE ?= avr-

MCU ?= attiny85
CFLAGS ?= -g -mmcu=$(MCU) -O3 -Werror -Woverflow
CPPFLAGS ?= -I$(SRCDIR) -DF_CPU=$(FREQ) -DSYNTH_CFG=\"poly_cfg.h\"
LDFLAGS ?= -mmcu=$(MCU) -O3 -Wl,--as-needed
PROG ?= avrdude
PROG_ARGS ?= -B 10 -c stk500v2 -P /dev/ttyACM0
PROG_DEV ?= t85
TARGET=$(BINDIR)/synth.ihex
INCLUDES += -I$(SRCDIR) -I$(PORTDIR)

LFUSE=0xc1
HFUSE=0xdf
EFUSE=0xff
FREQ=16000000

all: $(TARGET)

$(BINDIR)/%.pgm: $(BINDIR)/%.ihex
	$(PROG) $(PROG_ARGS) -p $(PROG_DEV) -U flash:w:$^:i

setfuse:
	$(PROG) $(PROG_ARGS) -p $(PROG_DEV) \
		-U lfuse:w:$(LFUSE):m \
		-U hfuse:w:$(HFUSE):m \
		-U efuse:w:$(EFUSE):m 
