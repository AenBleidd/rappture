CFLAGS	= -O2 -g
LDFLAGS =
TARGETS	= client mycat nanoscale
INSTALL_DIR	= /opt/nanoscale

all: $(TARGETS)

nanoscale: server.o
	cc $(CFLAGS) $(LDFLAGS) server.o  -o nanoscale

install: nanoscale
	cp nanoscale $(INSTALL_DIR)/bin

client: client.o clientlib.o

clean:
	rm -rf a.out *.o *~ core* $(TARGETS) *.log *.tmp logfile* .deps/*.d
