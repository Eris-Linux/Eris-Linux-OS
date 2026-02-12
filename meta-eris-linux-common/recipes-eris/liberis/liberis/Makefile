
LIBRARY  = liberis
VERSION  = 1.0.0
CFLAGS  += -Wall -g
LDLIBS  += -lcurl

TESTS = \
	test-liberis-gpios    \
	test-liberis-leds     \

LN ?= ln

all: $(LIBRARY).so

$(LIBRARY).o: $(LIBRARY).c $(LIBRARY).h
	$(CC) -c $(CFLAGS) -fPIC $<

$(LIBRARY).so.$(VERSION): $(LIBRARY).o
	$(CC) $(LDFLAGS) -shared -Wl,-soname,$(LIBRARY).so -o $(LIBRARY).so.$(VERSION) $^ $(LDLIBS)

$(LIBRARY).so: $(LIBRARY).so.$(VERSION)
	$(LN) -s  $(LIBRARY).so.$(VERSION)  $(LIBRARY).so

tests: $(TESTS)

%: %.c liberis.h
	$(CC) $(CFLAGS) -I. $(LDFLAGS) -L. $< -o $@ -leris

doc:
	doxygen  Doxyfile

clean:
	rm -rf out *.o $(LIBRARY).so* $(TESTS)
