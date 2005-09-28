# you need to change this to where your version of python is installed.
# tell make where to find python header files
PY_SRC_HEADERS	= /opt/rappture/include/python2.4

# tell make where to find the python shared library
EMB_PY_LIB_FLAG = -lpython2.4
EMB_PY_LIB_DIR 	= /opt/rappture/lib

EMB_PY_FLAGS 	= -Wl,-rpath,$(EMB_PY_LIB_DIR) -L$(EMB_PY_LIB_DIR) $(EMB_PY_LIB_FLAG)

# everything below should be ok, but check to make sure
# especially check the fortran flags, if youre using fortran
#
# define the top of our directory structure
# replace this with the full path of the directory
# containing the rappture directory
TOP_DIR         = ../..

# define the top of the rappture directory structure
RP_BASE   = $(TOP_DIR)/rappture

# define which programs can be made
PROGS			= 	librappture				\
					libRpObjects			\

# define our compiling environment
# 
CC 				= gcc
CPP				= g++
DEBUG	 		= -g -Wall
DEBUG_PLUS 		= -g -DDEBUG

# FORTRAN BINDINGS COMPILER FLAGS 
#
# available flags
#
# 	COMPNAME_NOCHANGE		- 	No change to the Rappture Library function 
# 								name
# 	COMPNAME_UPPERCASE  	-	Replace the Rappture Library function name 
# 								with an all uppercase version of the name
# 	COMPNAME_ADD1UNDERSCORE	-	add 1 underscore to the end of the Rappture 
# 								Library function name
# 	COMPNAME_ADD2UNDERSCORE	-	add 2 underscores to the end of the Rappture 
# 								Library function name
# 
# when setting CFLAGS, use the following guide for help
#
# gnu's g77/f77			COMPNAME_ADD2UNDERSCORE
# absoft's f77			COMPNAME_ADD1UNDERSCORE
# intel's ifort 		COMPNAME_ADD1UNDERSCORE
# intel's mpif90		COMPNAME_ADD1UNDERSCORE
#
#

CFLAGS 			= -DCOMPNAME_ADD2UNDERSCORE

F77		        = g77
#F77				= f77
#F77				= ifort
#F77				= /opt/mpich-1.2.6/p4-intel/bin/mpif90

LN              = ln

# define our directories
#
INCLUDES_DIR 	= $(RP_BASE)/include
BIN_DIR			= $(RP_BASE)/bin
LIB_DIR			= $(RP_BASE)/src
SRC_DIR			= $(RP_BASE)/src
TEST_DIR		= $(RP_BASE)/test

CORE_SRC 		= $(SRC_DIR)/core
FORT_SRC		= $(SRC_DIR)/fortran
CEE_SRC			= $(SRC_DIR)/cee
PY_SRC			= $(SRC_DIR)/python

LIB_INC_PREFIX	= -Wl,-rpath,$(LIB_DIR) -L$(LIB_DIR)
LIB_RP_OBJECTS  = $(LIB_INC_PREFIX) -lRpObjects
LIB_RAPPTURE 	= -Wl,-rpath,$(LIB_DIR) -L$(LIB_DIR) -lrappture

INCL_CORE 		= -I $(INCLUDES_DIR)/core
INCL_CEE		= -I $(INCLUDES_DIR)/cee -I $(PY_SRC_HEADERS)
INCL_FORTRAN	= -I $(INCLUDES_DIR)/fortran
INCL_PY			= -I $(INCLUDES_DIR)/python


# default: 

all: ${PROGS}

# include rappture library definitions

#### librappture shared object ###########################################

librappture: rappture_fortran.o rappture_interface.o RpUnitsCInterface.o RpUnitsStd.o RpUnits.o
	$(CPP) $(DEGUG) -shared -Wl,-rpath,$(LIB_DIR)/ \
		-Wl,-soname,$@.so -o $(LIB_DIR)/$@.so.0.0 $^ $(EMB_PY_FLAGS)

	/sbin/ldconfig -n $(LIB_DIR)


#### libRpObjects ########################################################
libRpObjects: RpVariable.o RpAbout.o RpNumber.o RpString.o RpBoolean.o RpChoice.o RpOption.o RpUnitsStd.o RpUnits.o
	$(CPP) $(DEGUG) -shared -Wl,-rpath,$(LIB_DIR)/ \
		-Wl,-soname,$@.so -o $(LIB_DIR)/$@.so.0.0 $^ -lc
	
	/sbin/ldconfig -n $(LIB_DIR)


# include core source files

Rp%.o: $(CORE_SRC)/Rp%.cc 
	$(CPP) -fPIC $(DEBUG) $(INCL_CORE) -o $@ -c $?

# include cee binding definitions

rappture_interface.o: $(CEE_SRC)/rappture_interface.c
	$(CPP) -fPIC $(DEBUG) $(INCL_CEE) $(INCL_PY) -o $@ -c $<

RpUnitsCInterface.o: $(CEE_SRC)/RpUnitsCInterface.cc
	$(CPP) $(CFLAGS) -fPIC $(DEBUG) $(INCL_CORE) $(INCL_CEE) -o $@ -c $?

# include fortran binding definitions

RpUnits_fortran.o: $(FORT_SRC)/RpUnits_fortran.c
	$(CPP) $(CFLAGS) -fPIC $(DEBUG) $(INCL_CORE) -o $@ -c $?

rappture_fortran.o: $(FORT_SRC)/rappture_fortran.c
	$(CPP) $(CFLAGS) -fPIC $(DEBUG) $(INCL_CORE) $(INCL_CEE) $(INCL_PY) -o $@ -c $<

#### CLEAN UP ############################################################
clean: 
	- rm -f $(BIN_DIR)/* *.o librappture.* libR*.so* 
