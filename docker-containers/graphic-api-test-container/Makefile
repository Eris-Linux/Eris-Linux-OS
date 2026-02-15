EXE=graphic-api-test

OBJS=                     \
  graphic-api-test.o      \
  virtual-keyboard.o      \
  gpio-api-window.o       \
  license-window.o        \
  network-api-window.o    \
  sbom-api-window.o       \
  system-api-window.o     \
  time-api-window.o       \
  update-api-window.o     \
  watchdog-api-window.o   \


LDFLAGS=$(shell pkg-config --libs Qt6Widgets)
CXXFLAGS=$(shell pkg-config --cflags Qt6Widgets)

.PHONY: all

all: $(EXE)

$(EXE): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ -leris

%o: %cpp
	$(CXX) -c $(CXXFLAGS) $<

.PHONY: clean

clean:
	rm -f $(EXE) $(OBJS)

