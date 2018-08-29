# build helloworld executable when user executes "make"
#dabai: dabai.o bluetooth.o uart.o gpio.o network.o subproc.o hsocket.o
#	$(CC) $(LDFLAGS) dabai.o bluetooth.o uart.o gpio.o network.o subproc.o hsocket.o -o dabai
#dabai.o: dabai.c
#	$(CC) $(CFLAGS) -c dabai.c
#bluetooth.o: bluetooth.c
#	$(CC) $(CFLAGS) -c bluetooth.c
#uart.o: uart.c
#	$(CC) $(CFLAGS) -c uart.c
#gpio.o: gpio.c
#	$(CC) $(CFLAGS) -c gpio.c
#network.o: network.c
#	$(CC) $(CFLAGS) -c network.c
#subproc.o: subproc.c
#	$(CC) $(CFLAGS) -c subproc.c
#hsocket.o: hsocket.c
#	$(CC) $(CFLAGS) -c hsocket.c
dabai: dabai.o bluetooth.o uart.o gpio.o network.o subproc.o hsocket.o
	$(CC) $(CFLAGS) -o $@ $^ -lpthread
%.o: %.c dabai.h
	$(CC) $(CFLAGS) -c $<

# remove object files and executable when user executes "make clean"
clean:
	rm *.o dabai
