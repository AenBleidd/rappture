CC = gcc
CFLAGS = -O3

all:
	$(CC) $(CFLAGS) perftest.c -o perftest
	./mkindex handler.tcl server.tcl client.tcl log.tcl random.tcl \
		options.tcl protocols.tcl statemachine.tcl wonks.tcl \
		foreman.tcl
