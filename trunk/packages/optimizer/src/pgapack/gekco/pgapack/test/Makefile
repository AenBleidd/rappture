# Generated automatically from Makefile.in by configure.
CC          = mpicc
FC          = mpif77
PRECFLAGS   = -O 
PREFFLAGS   = -w
CPPFLAGS    = -I/home/nano/u86/gekco/DEVELOP_GA/pgapack/include -I/opt/mpich-1.2.6/p4-gcc/include -Dlinux -DWL=32 -DOPTIMIZE 
RM          = /bin/rm -f
LDFLAGS     = -s  -L/home/nano/u86/gekco/DEVELOP_GA/pgapack/lib/linux  -lpgaO  /opt/mpich-1.2.6/p4-gcc/lib/libmpich.a -lm
SHELL       = /bin/sh

#    "$@" expands to the target; "$?" expands to the dependency list
CFLAGS      = -o $@ $? $(PRECFLAGS)
FFLAGS      = -o $@ $? $(PREFFLAGS)

LINK.c      = @echo "  Compiling $@" ; $(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS)
LINK.f      = @if test -n "$(FC)"; \
                  then echo "  Compiling $@" ; $(FC) $(FFLAGS) $(LDFLAGS); \
              else \
                  echo "$@: Fortran compiler unavailable"; fi

default:
	@make instverf 

instverf: instverf.c
	$(LINK.c)

clean: 
	@$(RM) instverf instverf.o
