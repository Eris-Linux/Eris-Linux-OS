
CFLAGS += -Wall
LDLIBS += -lX11
EXE = eris-dashboard
OBJS = eris-dashboard.o \



all: $(EXE)

$(EXE): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)


%.o: %.c
	$(CC) -c $(CFLAGS) $<


clean:
	rm -f $(OBJS) $(EXE)
