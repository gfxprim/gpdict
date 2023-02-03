CFLAGS?=-W -Wall -Wextra -O2 -ggdb
CFLAGS+=$(shell gfxprim-config --cflags)
BIN=gpdict
$(BIN): LDLIBS=-lgfxprim $(shell gfxprim-config --libs-widgets) -lstardict
SOURCES=$(wildcard *.c)
DEP=$(SOURCES:.c=.dep)
OBJ=$(SOURCES:.c=.o)

all: $(BIN) $(DEP)

%.dep: %.c
	$(CC) $(CFLAGS) -M $< -o $@

$(BIN): $(OBJ)

-include: $(DEP)

install:
	install -d $(DESTDIR)/etc/gp_apps/$(BIN)/
	install -m 644 layout.json -t $(DESTDIR)/etc/gp_apps/$(BIN)/
	install -m 644 layout_download.json -t $(DESTDIR)/etc/gp_apps/$(BIN)/
	install -m 644 dict_urls.json -t $(DESTDIR)/etc/gp_apps/$(BIN)/
	install -d $(DESTDIR)/usr/bin/
	install $(BIN) -t $(DESTDIR)/usr/bin/
	install -d $(DESTDIR)/usr/share/applications/
	install -m 644 $(BIN).desktop -t $(DESTDIR)/usr/share/applications/
	install -d $(DESTDIR)/usr/share/$(BIN)/
	install -m 644 $(BIN).png -t $(DESTDIR)/usr/share/$(BIN)/

clean:
	rm -f $(BIN) *.dep *.o

