
APP = eris-api-test

OBJS =               \
  eris-api-test.o    \
  gpio-api-test.o    \
  network-api-test.o \
  sbom-api-test.o    \
  system-api-test.o  \
  time-api-test.o    \
  update-api-test.o  \
  wdog-api-test.o    \

INC = liberis.h      \
  eris-api-test.h    \
  gpio-api-test.h    \
  network-api-test.h \
  sbom-api-test.h    \
  system-api-test.h  \
  time-api-test.h    \
  update-api-test.h  \
  wdog-api-test.h    \

CFLAGS  += -Wall -g
LDLIBS  += -leris

all: $(APP)

$(APP): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS)  $^ -o $@ $(LDLIBS)

%o: %c $(INC)
	$(CC) -c $(CFLAGS) $<

clean:
	rm -rf out *.o $(APP)
