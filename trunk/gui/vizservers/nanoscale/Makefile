CFLAGS	= -g
LDFLAGS =
TARGETS	= server client mycat
INSTALL_DIR	= /opt/nanovis

all: $(TARGETS)

install: server
	cp server $(INSTALL_DIR)/bin/nanoscale

client: client.o clientlib.o

clean:
	rm -rf a.out *.o *~ core* $(TARGETS) *.log *.tmp logfile* .deps/*.d
