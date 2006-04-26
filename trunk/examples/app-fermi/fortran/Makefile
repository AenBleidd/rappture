# Makefile for app-fermi/fortran example
#
# Replace this with the location of your rappture installation:
ifndef RAPPTURE_DIR
RAPPTURE_DIR = /opt/rappture
endif

# no need to edit the rest...
PROGS = fermi

#F77 = ifort
#F77 = /opt/mpich-1.2.6/p4-intel/bin/mpif90
F77 = g77

LIB_DIR = $(RAPPTURE_DIR)/lib
LIB_RAPPTURE = -Wl,-rpath,$(LIB_DIR) -L$(LIB_DIR) -lrappture

all: $(PROGS)

fermi: fermi.f
	if test "`uname`" == "Darwin"; then \
		$(F77) -g -L$(LIB_DIR) -lrappture -lscew -o $@ $< ;\
	else \
		$(F77) -g -Wall $(LIB_RAPPTURE) -o $@ $< ;\
	fi


clean: 
	rm -f *.o $(PROGS) run*.xml
