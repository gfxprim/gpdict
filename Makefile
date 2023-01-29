CFLAGS?=-W -Wall -Wextra -O2
CFLAGS+=$(shell gfxprim-config --cflags)
BIN=gpdict
$(BIN): LDLIBS=-lgfxprim $(shell gfxprim-config --libs-widgets) -lstardict
SOURCES=$(wildcard *.c)
DEP=$(SOURCES:.c=.dep)

all: $(BIN) $(DEP)

%.dep: %.c
	$(CC) $(CFLAGS) -M $< -o $@

-include: $(DEP)

clean:
	rm -f $(BIN) *.dep *.o

