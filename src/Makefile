# you need to change this to where your version of python is installed.
# tell make where to find python header files
RP_INSTALL_BASE = /opt/rappture

# tell make where to find the expat & libscew sources
INCL_RP_DEPS    = -I $(RP_INSTALL_BASE)/include

#EXPAT_HEADERS   = $(RP_INSTALL_BASE)/include
#LIB_EXPAT_INCL  = -I $(EXPAT_HEADERS)
#SCEW_HEADERS    = $(RP_INSTALL_BASE)/include/scew
#LIB_SCEW_INCL   = -I $(SCEW_HEADERS) -I $(EXPAT_HEADERS)

LIB_SCEW_FLAG   = -L$(RP_INSTALL_BASE)/lib -lscew

#LIB_SCEW_FLAG   = /opt/rappture/lib/libscew.a
#LIB_SCEW_FLAG   = -static -L/opt/rappture/lib -lscew


# everything below should be ok, but check to make sure
#
# define the top of our directory structure
# replace this with the full path of the directory
# containing the rappture directory
TOP_DIR         = ../..

# define the top of the rappture directory structure
RP_BASE   = $(TOP_DIR)/rappture

# define which programs can be made
PROGS           =   librappture   \
                    libRpObjects  \
                    Rappture.Units

# define our compiling environment
# 
CC              = gcc
CXX             = g++
DEBUG           = -g -Wall
DEBUG_PLUS      = -g -DDEBUG
PY              = python

LN              = ln

# define our directories
#
INCLUDES_DIR    = $(RP_BASE)/include
BIN_DIR         = $(RP_BASE)/bin
LIB_DIR         = $(RP_BASE)/src
SRC_DIR         = $(RP_BASE)/src
TEST_DIR        = $(RP_BASE)/test

CORE_SRC        = $(SRC_DIR)/core
FORT_SRC        = $(SRC_DIR)/fortran
CEE_SRC         = $(SRC_DIR)/cee
PY_SRC          = $(SRC_DIR)/python

LIB_INC_PREFIX  = -Wl,-rpath,$(LIB_DIR) -L$(LIB_DIR)
LIB_RP_OBJECTS  = $(LIB_INC_PREFIX) -lRpObjects
LIB_RAPPTURE    = -Wl,-rpath,$(LIB_DIR) -L$(LIB_DIR) -lrappture

INCL_CORE       = -I $(INCLUDES_DIR)/core
INCL_CEE        = -I $(INCLUDES_DIR)/cee
INCL_FORTRAN    = -I $(INCLUDES_DIR)/fortran
INCL_PY         = -I $(INCLUDES_DIR)/python


# default:

all: ${PROGS}
install: all install_py install_rappture install_rpobjs

RP_IO_DEPS      = RpLibrary.o RpLibraryCInterface.o RpLibraryFInterface.o \
                  scew_extras.o
RP_UNITS_DEPS   = RpUnitsStd.o RpUnits.o RpUnitsCInterface.o RpUnitsFInterface.o
RP_OTHER_DEPS   = RpFortranCommon.o # RpBindingsDict.o
RP_OBJS_DEP     = RpVariable.o RpAbout.o RpNumber.o RpString.o RpBoolean.o \
                  RpChoice.o RpOption.o RpUnitsStd.o RpUnits.o

# include rappture library definitions

#### librappture shared object ###########################################

librappture: $(RP_IO_DEPS) $(RP_UNITS_DEPS) $(RP_OTHER_DEPS)
	$(CXX) $(DEGUG) -shared -Wl,-rpath,$(LIB_DIR)/ \
		-Wl,-soname,$@.so -o $(LIB_DIR)/$@.so.0.0 $^ $(LIB_SCEW_FLAG)

	/sbin/ldconfig -n $(LIB_DIR)

install_rappture: librappture
	cp -d $<.so* $(RP_INSTALL_BASE)/lib

#### libRpObjects ########################################################
libRpObjects: $(RP_OBJS_DEP)
	$(CC) $(DEGUG) -shared -Wl,-rpath,$(LIB_DIR)/ \
		-Wl,-soname,$@.so -o $(LIB_DIR)/$@.so.0.0 $^ -lstdc++
#	$(CXX) $(DEGUG) -shared -Wl,-rpath,$(LIB_DIR)/ \
#		-Wl,-soname,$@.so -o $(LIB_DIR)/$@.so.0.0 $^
	
	/sbin/ldconfig -n $(LIB_DIR)

install_rpobjs: libRpObjects
	cp -d $<.so* $(RP_INSTALL_BASE)/lib

#### Python ########################################################
Rappture.Units: RpUnits.o RpUnitsStd.o
	$(PY) $(PY_SRC)/setup.py build

install_py: RpUnits.o RpUnitsStd.o
	$(PY) $(PY_SRC)/setup.py install



# include core source files

RpLibrary.o: $(CORE_SRC)/RpLibrary.cc
	$(CXX) -fPIC $(DEBUG) $(INCL_CORE) $(INCL_RP_DEPS) -o $@ -c $?

scew_extras.o: $(CORE_SRC)/scew_extras.c
	$(CC) -fPIC $(DEBUG) $(INCL_CORE) $(INCL_RP_DEPS) -o $@ -c $?

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

RpBindingsDict.o: $(CORE_SRC)/RpBindingsDict.cc
	$(CXX) -fPIC $(DEBUG) $(INCL_CORE) -o $@ -c $?




# include cee binding definitions

rappture_interface.o: $(CEE_SRC)/rappture_interface.c
	$(CXX) -fPIC $(DEBUG) $(INCL_CEE) $(INCL_PY) -o $@ -c $<

RpUnitsCInterface.o: $(CEE_SRC)/RpUnitsCInterface.cc
	$(CXX) -fPIC $(DEBUG) $(INCL_CORE) $(INCL_CEE) -o $@ -c $?

RpLibraryCInterface.o: $(CEE_SRC)/RpLibraryCInterface.cc
	$(CXX) -fPIC $(DEBUG) $(INCL_CORE) $(INCL_CEE) $(INCL_RP_DEPS) -o $@ -c $?




# include fortran binding definitions

rappture_fortran.o: $(FORT_SRC)/rappture_fortran.c
	$(CXX) -fPIC $(DEBUG) $(INCL_FORTRAN) $(INCL_CORE) $(INCL_CEE) $(INCL_PY) -o $@ -c $<

RpUnitsFInterface.o: $(FORT_SRC)/RpUnitsFInterface.cc
	$(CXX) -fPIC $(DEBUG) $(INCL_CORE) $(INCL_FORTRAN) -o $@ -c $?

RpFortranCommon.o: $(FORT_SRC)/RpFortranCommon.c
	$(CXX) -fPIC $(DEBUG) $(INCL_FORTRAN) -o $@ -c $<

RpLibraryFInterface.o: $(FORT_SRC)/RpLibraryFInterface.cc
	$(CXX) -fPIC $(DEBUG) $(INCL_CORE) $(INCL_FORTRAN) $(INCL_RP_DEPS) -o $@ -c $?




#### CLEAN UP ############################################################
clean: 
	- rm -f *.o librappture.so* libRp*.so*
	- rm -rf build
