CFLAGS	= -g
LDFLAGS =
TARGETS	= server client mycat

all: $(TARGETS)

client: client.o clientlib.o

clean:
	rm -rf a.out *.o *~ core* $(TARGETS) *.log *.tmp logfile* .deps/*.d
