# Compiler definitions
CC = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
SIZE = $(CROSS_COMPILE)size

PORT ?= pc
BINDIR ?= bin/$(PORT)
PORTDIR ?= ports/$(PORT)
OBJDIR ?= obj/$(PORT)

SRCDIR ?= $(PWD)

.PHONY: setfuse all clean prepare

-include local.mk

all: $(TARGET)

prepare:
	for d in $(OBJDIR) $(BINDIR); do \
		[ -d $$d ] || mkdir -p $$d; \
	done

$(BINDIR)/synth: $(OBJDIR)/main.o $(OBJDIR)/poly.a
	$(CC) -g -o $@ $(LDFLAGS) $^

$(OBJDIR)/poly.a: $(OBJDIR)/adsr.o $(OBJDIR)/waveform.o
	$(AR) rcs $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.c prepare
	$(CC) -o $@ -c $(CPPFLAGS) $(CFLAGS) $<

$(OBJDIR)/%.o: $(PORTDIR)/%.c prepare
	$(CC) -o $@ -c $(CPPFLAGS) $(CFLAGS) $<

clean:
	-rm -fr $(OBJDIR) $(BINDIR)

$(BINDIR)/%.ihex: $(BINDIR)/%
	$(OBJCOPY) -j .text -j .data -O ihex $^ $@

include $(PORTDIR)/Makefile
