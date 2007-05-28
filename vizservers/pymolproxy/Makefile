TARGETS = pymolproxy

CC      = gcc
CFLAGS  = -g -O2 -I/usr/include/tcl8.4
LDFLAGS = 
# LDFLAGS = $(LDFLAGS)

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

pymolproxy: pymolproxy.o
	$(CC) $(CFLAGS) $(LDFLAGS) -DSTANDALONE pymolproxy.c -o pymolproxy -ltcl8.4

install: pymolproxy
	$(INSTALL_PROGRAM) pymolproxy $(bindir)/pymolproxy

clean:
	rm -rf a.out *.o *~ core* $(TARGETS) *.log *.tmp logfile* .deps/*.d

