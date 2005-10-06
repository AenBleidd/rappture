# you need to change this to where your version of python is installed.
# tell make where to find python header files
RP_INSTALL_BASE = /opt/rappture

# tell make where to find the rappture dependency include files
INCL_RP_DEPS    = -I $(RP_INSTALL_BASE)/include

# define the top of our directory structure
# replace this with the full path of the directory
# containing the rappture directory
TOP_DIR         = ../..

# define the top of the rappture directory structure
RP_BASE         = $(TOP_DIR)/rappture

# define which programs can be made

TEST_PROGS      = RpBoolean_test    \
                  RpChoice_test     \
                  RpNumber_test     \
                  RpString_test     \
                  RpUnits_test      \
                  RpVariable_test   \
                  RpUnitsC_test     \
                  RpUnitsF_test     \
                  RpLibrary_test    \
                  RpLibraryC_test   \
                  RpLibraryF_test

# define our compiling environment
#
CC              = gcc
CXX             = g++
DEBUG           = -g -Wall
DEBUG_PLUS      = -g -DDEBUG

PY              = python

F77             = g77

LN              = ln

# define our directories
#
INCLUDES_DIR    = $(RP_BASE)/include
BIN_DIR         = $(RP_BASE)/bin
LIB_DIR         = $(RP_INSTALL_BASE)/lib # $(RP_BASE)/src
SRC_DIR         = $(RP_BASE)/src
TEST_DIR        = $(RP_BASE)/test

LIB_INC_PREFIX  = -Wl,-rpath,$(LIB_DIR) -L$(LIB_DIR)

LIB_RP_OBJECTS  = $(LIB_INC_PREFIX) -lRpObjects
LIB_RAPPTURE    = -Wl,-rpath,$(LIB_DIR) -L$(LIB_DIR) -lrappture

INCL_CORE       = -I $(INCLUDES_DIR)/core
INCL_CEE        = -I $(INCLUDES_DIR)/cee
INCL_FORTRAN    = -I $(INCLUDES_DIR)/fortran
INCL_PY         = -I $(INCLUDES_DIR)/python


default: $(TEST_PROGS)
all: $(TEST_PROGS)
test: $(TEST_PROGS)

# include test programs

SRC_TEST            = $(TEST_DIR)/src


RpBoolean_test: $(SRC_TEST)/RpBoolean_test.cc
	$(CXX) $(DEBUG) $(INCL_CORE) -o $@ $< $(LIB_RP_OBJECTS)
    
RpChoice_test: $(SRC_TEST)/RpChoice_test.cc
	$(CXX) $(DEBUG) $(INCL_CORE) -o $@ $< $(LIB_RP_OBJECTS)
    
RpNumber_test: $(SRC_TEST)/RpNumber_test.cc
	$(CXX) $(DEBUG) $(INCL_CORE) -o $@ $< $(LIB_RP_OBJECTS)

RpString_test: $(SRC_TEST)/RpString_test.cc
	$(CXX) $(DEBUG) $(INCL_CORE) -o $@ $< $(LIB_RP_OBJECTS)

RpUnits_test: $(SRC_TEST)/RpUnits_test.cc
	$(CXX) $(DEBUG) $(INCL_CORE) -o $@ $< $(LIB_RAPPTURE)

RpVariable_test: $(SRC_TEST)/RpVariable_test.cc
	$(CXX) $(DEBUG) $(INCL_CORE) -o $@ $< $(LIB_RP_OBJECTS)

RpUnitsC_test: $(SRC_TEST)/RpUnitsC_test.c
	$(CC) $(DEBUG) $(INCL_CEE) $(INCL_CORE) -o $@ $^ $(LIB_RAPPTURE)

RpLibrary_test: $(SRC_TEST)/RpLibrary_test.cc
	$(CXX) $(DEBUG) $(INCL_CEE) $(INCL_CORE) $(INCL_RP_DEPS) -o $@ $^ $(LIB_RAPPTURE)

RpLibraryC_test: $(SRC_TEST)/RpLibraryC_test.c
	$(CC) $(DEBUG) $(INCL_CEE) $(INCL_CORE) $(INCL_RP_DEPS) -o $@ $^ $(LIB_RAPPTURE)

RpLibraryF_test: $(SRC_TEST)/RpLibraryF_test.f
	$(F77) $(DEBUG) -o $@ $^ $(LIB_RAPPTURE)

RpUnitsF_test: $(SRC_TEST)/RpUnitsF_test.f
	$(F77) $(DEBUG) -o $@ $^ $(LIB_RAPPTURE)


#### CLEAN UP ############################################################
clean: 
	- rm -f *.o $(TEST_PROGS) run*.xml
