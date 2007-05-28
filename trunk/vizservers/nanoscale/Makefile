TARGETS         = client mycat nanoscale

CC              = gcc
CFLAGS          = -g -O2
LDFLAGS         = 

prefix          = /home/nanohub/dkearney/repo/viz_20070521
exec_prefix     = /home/nanohub/dkearney/repo/viz_20070521
bindir          = ${exec_prefix}/bin
libdir          = ${exec_prefix}/lib
includedir      = ${prefix}/include
mandir          = ${prefix}/man

INSTALL         = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL} -m 755
INSTALL_DATA    = ${INSTALL} -m 644
INSTALL_SCRIPT  = ${INSTALL} -m 644

.PHONY: all install clean

all: $(TARGETS)

nanoscale: server.o
	$(CC) $(CFLAGS) $(LDFLAGS) server.o -o nanoscale

install: nanoscale
	$(INSTALL_PROGRAM) -D nanoscale $(bindir)/nanoscale

client: client.o clientlib.o

clean:
	rm -rf a.out *.o *~ core* $(TARGETS) *.log *.tmp logfile* .deps/*.d
