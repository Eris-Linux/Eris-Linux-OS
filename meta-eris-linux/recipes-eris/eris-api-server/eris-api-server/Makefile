## Makefile - Eris-Linux API.
##
## Eris-Linux team 2025.
##
## License GPL.

CC ?= gcc
CFLAGS += -Wall -pthread -g
LDFLAGS += -pthread -g
LIBS += -lgpiod -luuid

EXE = eris-api-server
OBJS =              \
	api-server.o    \
	gpio-api.o      \
	leds-api.o      \
	net-api.o       \
	procfs-api.o    \
	sbom-api.o      \
	system-api.o    \
	time-api.o      \
	update-api.o    \
	wdog-api.o      \

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
