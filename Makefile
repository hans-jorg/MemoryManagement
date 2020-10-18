
PROGNAME=testmemmanager
CFLAGS += -g -DTEST -DDEBUG


$(PROGNAME): memmanager.o
	$(CC) -o $@ $(CFLAGS) $^ $(LFLAGS) $(LIBS)

clean:
	rm -rf $(PROGNAME) *.o
