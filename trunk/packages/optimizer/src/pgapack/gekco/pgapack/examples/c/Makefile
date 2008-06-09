# Generated automatically from Makefile.in by configure.
CC          = mpicc
PRECFLAGS   = -O 
CPPFLAGS    = -I/home/nano/u86/gekco/DEVELOP_GA/pgapack/include -I/opt/mpich-1.2.6/p4-gcc/include -Dlinux -DWL=32 -DOPTIMIZE 
FC          = mpif77
RM          = /bin/rm -f
LDFLAGS     = -s  -L/home/nano/u86/gekco/DEVELOP_GA/pgapack/lib/linux  -lpgaO  /opt/mpich-1.2.6/p4-gcc/lib/libmpich.a -lm
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

clean: 
	@$(RM) classic dejong example maxbit maxchar maxint name namefull \
	       udtstr *.o
