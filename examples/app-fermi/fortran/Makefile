# Makefile for app-fermi/fortran example
#
# Replace this with the location of your rappture installation:
RAPPTURE_DIR = /opt/rappture

# no need to edit the rest...
PROGS =	fermi

CFLAGS = -DCOMPNAME_ADD2UNDERSCORE

#F77 = ifort
#F77 = /opt/mpich-1.2.6/p4-intel/bin/mpif90
F77 = g77

LIB_DIR = $(RAPPTURE_DIR)/lib
LIB_RAPPTURE	= -Wl,-rpath,$(LIB_DIR) -L$(LIB_DIR) -lrappture

all: $(PROGS)

fermi: fermi.f
	$(F77) -g -Wall $(LIB_RAPPTURE) -o $@ $<

clean: 
	rm -f *.o $(PROGS)
