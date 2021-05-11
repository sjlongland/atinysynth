CROSS_COMPILE ?=

CFLAGS ?= -g -Werror -Woverflow
CPPFLAGS ?= -I$(SRCDIR) -I$(PORTDIR)
LDFLAGS ?= -g -lao -lm -Wl,--as-needed
LIBS += -lao -lm
INCLUDES += -I$(SRCDIR) -I$(PORTDIR)
OBJECTS += $(OBJDIR)/main.o

TARGET=$(BINDIR)/synth

all: $(TARGET)
