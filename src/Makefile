# you need to change this to where your version of python is installed.
# tell make where to find python header files
PY_SRC_HEADERS	= /opt/rappture/include/python2.4

# tell make where to find the python shared library
EMB_PY_LIB_FLAG = -lpython2.4
EMB_PY_LIB_DIR 	= /opt/rappture/lib

EMB_PY_FLAGS 	= -Wl,-rpath,$(EMB_PY_LIB_DIR) -L$(EMB_PY_LIB_DIR) $(EMB_PY_LIB_FLAG)


# tell make where to find the libscew sources
SCEW_HEADERS	= /opt/rappture/include/scew
LIB_SCEW_INCL	= -I $(SCEW_HEADERS)
LIB_SCEW_FLAG	= -L/opt/rappture/lib -lscew
#LIB_SCEW_FLAG	= /opt/rappture/lib/libscew.a
#LIB_SCEW_FLAG	= -static -L/opt/rappture/lib -lscew


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
					librappture_test

# define our compiling environment
# 
CC 				= gcc
CXX				= g++
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
LIB_RAPPTURE_T 	= -Wl,-rpath,$(LIB_DIR) -L$(LIB_DIR) -lrappture_test

INCL_CORE 		= -I $(INCLUDES_DIR)/core
INCL_CEE		= -I $(INCLUDES_DIR)/cee -I $(PY_SRC_HEADERS)
INCL_FORTRAN	= -I $(INCLUDES_DIR)/fortran
INCL_PY			= -I $(INCLUDES_DIR)/python


# default: 

all: ${PROGS}

RP_IO_DEPS 		= RpLibrary.o RpLibraryCInterface.o RpLibraryFInterface.o scew_extras.o 
RP_UNITS_DEPS 	= RpUnitsStd.o RpUnits.o RpUnitsCInterface.o RpUnitsFInterface.o
RP_OTHER_DEPS	= RpFortranCommon.o

# include rappture library definitions

#### librappture shared object ###########################################

librappture: rappture_fortran.o rappture_interface.o RpUnitsCInterface.o RpUnitsFInterface.o RpFortranCommon.o RpUnitsStd.o RpUnits.o
	$(CXX) $(DEGUG) -shared -Wl,-rpath,$(LIB_DIR)/ \
		-Wl,-soname,$@.so -o $(LIB_DIR)/$@.so.0.0 $^ $(EMB_PY_FLAGS)

	/sbin/ldconfig -n $(LIB_DIR)

librappture_test: $(RP_IO_DEPS) $(RP_UNITS_DEPS) $(RP_OTHER_DEPS)
	$(CXX) $(DEGUG) -shared -Wl,-rpath,$(LIB_DIR)/ \
		-Wl,-soname,$@.so -o $(LIB_DIR)/$@.so.0.0 $^ $(LIB_SCEW_FLAG)

	/sbin/ldconfig -n $(LIB_DIR)

#### libRpObjects ########################################################
libRpObjects: RpVariable.o RpAbout.o RpNumber.o RpString.o RpBoolean.o RpChoice.o RpOption.o RpUnitsStd.o RpUnits.o
	$(CC) $(DEGUG) -shared -Wl,-rpath,$(LIB_DIR)/ \
		-Wl,-soname,$@.so -o $(LIB_DIR)/$@.so.0.0 $^ -lstdc++
#	$(CXX) $(DEGUG) -shared -Wl,-rpath,$(LIB_DIR)/ \
#		-Wl,-soname,$@.so -o $(LIB_DIR)/$@.so.0.0 $^
	
	/sbin/ldconfig -n $(LIB_DIR)


# include core source files

RpLibrary.o: $(CORE_SRC)/RpLibrary.cc 
	$(CXX) -fPIC $(DEBUG) $(INCL_CORE) $(LIB_SCEW_INCL) -o $@ -c $?

scew_extras.o: $(CORE_SRC)/scew_extras.c 
	$(CC) -fPIC $(DEBUG) $(INCL_CORE) $(LIB_SCEW_INCL) -o $@ -c $?

RpVariable.o: $(CORE_SRC)/RpVariable.cc 
	$(CXX) -fPIC $(DEBUG) $(INCL_CORE) -o $@ -c $?

RpAbout.o: $(CORE_SRC)/RpAbout.cc 
	$(CXX) -fPIC $(DEBUG) $(INCL_CORE) -o $@ -c $?

RpNumber.o: $(CORE_SRC)/RpNumber.cc 
	$(CXX) -fPIC $(DEBUG) $(INCL_CORE) -o $@ -c $?

RpString.o: $(CORE_SRC)/RpString.cc 
	$(CXX) -fPIC $(DEBUG) $(INCL_CORE) -o $@ -c $?

RpBoolean.o: $(CORE_SRC)/RpBoolean.cc 
	$(CXX) -fPIC $(DEBUG) $(INCL_CORE) -o $@ -c $?

RpChoice.o: $(CORE_SRC)/RpChoice.cc 
	$(CXX) -fPIC $(DEBUG) $(INCL_CORE) -o $@ -c $?

RpOption.o: $(CORE_SRC)/RpOption.cc 
	$(CXX) -fPIC $(DEBUG) $(INCL_CORE) -o $@ -c $?

RpUnitsStd.o: $(CORE_SRC)/RpUnitsStd.cc 
	$(CXX) -fPIC $(DEBUG) $(INCL_CORE) -o $@ -c $?

RpUnits.o: $(CORE_SRC)/RpUnits.cc 
	$(CXX) -fPIC $(DEBUG) $(INCL_CORE) -o $@ -c $?




# include cee binding definitions

rappture_interface.o: $(CEE_SRC)/rappture_interface.c
	$(CXX) -fPIC $(DEBUG) $(INCL_CEE) $(INCL_PY) -o $@ -c $<

RpUnitsCInterface.o: $(CEE_SRC)/RpUnitsCInterface.cc
	$(CXX) -fPIC $(DEBUG) $(INCL_CORE) $(INCL_CEE) -o $@ -c $?

RpLibraryCInterface.o: $(CEE_SRC)/RpLibraryCInterface.cc 
	$(CXX) -fPIC $(DEBUG) $(INCL_CORE) $(INCL_CEE) $(LIB_SCEW_INCL) -o $@ -c $?




# include fortran binding definitions

rappture_fortran.o: $(FORT_SRC)/rappture_fortran.c
	$(CXX) $(CFLAGS) -fPIC $(DEBUG) $(INCL_FORTRAN) $(INCL_CORE) $(INCL_CEE) $(INCL_PY) -o $@ -c $<

RpUnitsFInterface.o: $(FORT_SRC)/RpUnitsFInterface.cc
	$(CXX) $(CFLAGS) -fPIC $(DEBUG) $(INCL_CORE) $(INCL_FORTRAN) -o $@ -c $?

RpFortranCommon.o: $(FORT_SRC)/RpFortranCommon.c
	$(CXX) $(CFLAGS) -fPIC $(DEBUG) $(INCL_FORTRAN) -o $@ -c $<

RpLibraryFInterface.o: $(FORT_SRC)/RpLibraryFInterface.cc 
	$(CXX) $(CFLAGS) -fPIC $(DEBUG) $(INCL_CORE) $(INCL_FORTRAN) $(LIB_SCEW_INCL) -o $@ -c $?




#### CLEAN UP ############################################################
clean: 
	- rm -f *.o librappture*.so* libRp*.so*
