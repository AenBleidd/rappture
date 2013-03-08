FC              = gfortran
FCFLAGS         =
FCDEBUGFLAGS    = -Wall -g
INCLUDES        = -I/apps/rappture/dev/include
LIBS            = -L/apps/rappture/dev/lib -lrappture -lexpat -lz -lm

FC_SWITCHES     = $(FCFLAGS) $(FCDEBUGFLAGS) $(INCLUDES)

.PHONY: all install clean distclean

all: showdata

showdata: showdata.f
	$(FC) $(FC_SWITCHES) $< -o $@ $(LIBS)

install: all

clean:
	$(RM) showdata run*.xml

distclean: clean

