## Makefile - Eris-Linux Rest API.
##
## Eris-Linux team 2025-2026.
##
## License GPL.

CC ?= gcc
CFLAGS += -Wall -pthread -g
LDFLAGS += -pthread -g
LIBS += -lgpiod -lmicrohttpd -luuid

EXE = eris-rest-api
OBJS =                 \
    addsnprintf.o      \
    eris-rest-api.o    \
    gpio-rest-api.o    \
    net-rest-api.o     \
    sbom-rest-api.o    \
    system-rest-api.o  \
    time-rest-api.o    \
    update-rest-api.o  \
    wdog-rest-api.o    \

INC =


DESTDIR ?= /usr/sbin

.PHONY: all

all: $(EXE)

$(EXE): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.c $(INC)
	$(CC) $(CFLAGS) -c $<

.PHONY: clean

clean:
	rm -f *.o $(EXE)

.PHONY: install

install: $(EXE)
	cp $(EXE) $(DESTDIR)/
