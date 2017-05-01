CROSS_COMPILE ?=

CFLAGS ?= -g -Werror -Woverflow
CPPFLAGS ?= -I$(SRCDIR) -I$(PORTDIR)
LDFLAGS ?= -g -lao -Wl,--as-needed
LIBS += -lao
INCLUDES += -I$(SRCDIR) -I$(PORTDIR)
OBJECTS += $(OBJDIR)/main.o

TARGET=$(BINDIR)/synth

all: $(TARGET)
