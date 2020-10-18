
PROGNAME=testmemmanager
CFLAGS += -g -DTEST -DDEBUG


$(PROGNAME): memmanager.o
	$(CC) -o $@ $(CFLAGS) $^ $(LFLAGS) $(LIBS)

run: $(PROGNAME)
	./$(PROGNAME)

docs:
	doxygen

clean:
	rm -rf $(PROGNAME) *.o html latex
