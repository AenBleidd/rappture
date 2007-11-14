# Generated automatically from Makefile.in by configure.
CC          = cc
PRECFLAGS   = -O 
CPPFLAGS    = -I/Volumes/Data_mo/GENES/Software/pgapack/include -I/Volumes/Data_mo/GENES/Software/mpich-1.2.7p1/include/ -Dlinux -DWL=32 -DFORTRANUNDERSCORE -DOPTIMIZE 
FC          = f77
RM          = /bin/rm -f
LDFLAGS     = -s  -L/Volumes/Data_mo/GENES/Software/pgapack/lib/linux  -lpgaO  /Volumes/Data_mo/GENES/Software/mpich-1.2.7p1/lib/libmpich.a -lm
SHELL       = /bin/sh

#    "$@" expands to the target; "$?" expands to the dependency list
CFLAGS      = -o $@ $? $(PRECFLAGS)

LINK.c      = @echo "  Compiling $@" ; $(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS)

default:
	@make classic
	@make dejong
	@make example
	@make maxbit
	@make maxchar
	@make maxint
	@make name
	@make namefull
	@make udtstr
	@make sampleOptimization

classic: classic.c
	$(LINK.c)

dejong: dejong.c
	$(LINK.c)

example: example.c
	$(LINK.c)

maxbit: maxbit.c
	$(LINK.c)

maxchar: maxchar.c
	$(LINK.c)

maxint: maxint.c
	$(LINK.c)

name: name.c
	$(LINK.c)

namefull: namefull.c
	$(LINK.c)

udtstr: udtstr.c
	$(LINK.c)

sampleOptimization: sampleOptimization.c
	$(LINK.c)
clean: 
	@$(RM) classic dejong example maxbit maxchar maxint name namefull \
	       udtstr *.o



