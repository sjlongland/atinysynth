# Compiler definitions
CC = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
SIZE = $(CROSS_COMPILE)size

INCLUDES ?=
LIBS ?=
PORT ?= pc
BINDIR ?= bin/$(PORT)
PORTDIR ?= ports/$(PORT)
OBJDIR ?= obj/$(PORT)
OBJECTS :=

SRCDIR ?= $(PWD)

.PHONY: setfuse all clean

-include local.mk
include $(PORTDIR)/Makefile

$(BINDIR)/synth: $(OBJECTS) $(OBJDIR)/poly.a
	@[ -d $(BINDIR) ] || mkdir -p $(BINDIR)
	$(CC) -g -o $@ $(LDFLAGS) $(LIBS) $^

$(OBJDIR)/poly.a: $(OBJDIR)/adsr.o $(OBJDIR)/waveform.o $(OBJDIR)/mml.o $(OBJDIR)/sequencer.o
	$(AR) rcs $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@[ -d $(OBJDIR) ] || mkdir -p $(OBJDIR)
	$(CC) -o $@ -c $(CPPFLAGS) $(INCLUDES) $(CFLAGS) $<
	$(CC) -MM $(CPPFLAGS) $(INCLUDES) $< \
		| sed -e '/^[^ ]\+:/ s:^:$(OBJDIR)/:g' > $@.dep

$(OBJDIR)/%.o: $(PORTDIR)/%.c
	@[ -d $(OBJDIR) ] || mkdir -p $(OBJDIR)
	$(CC) -o $@ -c $(CPPFLAGS) $(INCLUDES) $(CFLAGS) $<
	$(CC) -MM $(CPPFLAGS) $(INCLUDES) $< \
		| sed -e '/^[^ ]\+:/ s:^:$(OBJDIR)/:g' > $@.dep

$(SRCDIR)/sine.c: $(SRCDIR)/gensine.py
	python $^ --amplitude 127 --num-samples-bits \
		--num-samples 6 --data-type int8_t \
		> $@

clean:
	-rm -fr $(OBJDIR) $(BINDIR)

$(BINDIR)/%.ihex: $(BINDIR)/%
	$(OBJCOPY) -j .text -j .data -O ihex $^ $@

-include $(OBJDIR)/*.dep
