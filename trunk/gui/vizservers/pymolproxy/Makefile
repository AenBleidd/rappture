CFLAGS	= -g -I/usr/include/tcl8.4
LDFLAGS = 
TARGETS	= pymolproxy
INSTALL_DIR	= /opt/pymolproxy

all: $(TARGETS)

install: pymolproxy
	cp pymolproxy $(INSTALL_DIR)/bin/pymolproxy

pymolproxy: pymolproxy.o
	cc $(CFLAGS) -DSTANDALONE pymolproxy.c -o pymolproxy -ltcl8.4

clean:
	rm -rf a.out *.o *~ core* $(TARGETS) *.log *.tmp logfile* .deps/*.d

