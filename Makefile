
PROGNAME=testmemmanager
CFLAGS += -g -DTEST -DDEBUG


$(PROGNAME): memmanager.o
	$(CC) -o $@ $(CFLAGS) $^ $(LFLAGS) $(LIBS)

docs:
	doxygen

clean:
	rm -rf $(PROGNAME) *.o html latex
