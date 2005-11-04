# you need to change this to where your version of python is installed.
# tell make where to find python header files
RP_INSTALL_BASE = /opt/rappture

# tell make where to find the expat & libscew sources
INCL_RP_DEPS    = -I$(RP_INSTALL_BASE)/include

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
PROGS           =   librappture    \
                    libRpObjects   \
                    Rappture.Units
#                    RpMatlab

# define our compiling environment
#
CC              = gcc
CXX             = g++
DEBUG           = -g -Wall
DEBUG_PLUS      = -g -DDEBUG
PY              = python
MEX             = mex
OCT             = mkoctfile

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
MATLAB_SRC      = $(SRC_DIR)/matlab
OCTAVE_SRC      = $(SRC_DIR)/octave

LIB_INC_PREFIX  = -Wl,-rpath,$(LIB_DIR) -L$(LIB_DIR)
LIB_RP_OBJECTS  = $(LIB_INC_PREFIX) -lRpObjects
LIB_RAPPTURE    = -Wl,-rpath,$(LIB_DIR) -L$(LIB_DIR) -lrappture

INCL_CORE       = -I$(INCLUDES_DIR)/core
INCL_CEE        = -I$(INCLUDES_DIR)/cee
INCL_FORTRAN    = -I$(INCLUDES_DIR)/fortran
INCL_PY         = -I$(INCLUDES_DIR)/python
INCL_MATLAB     = -I$(INCLUDES_DIR)/matlab
INCL_OCTAVE     = -I$(INCLUDES_DIR)/octave


# default:

all: ${PROGS}
install: all install_py install_rappture install_rpobjs #install_matlab #install_octave

RP_IO_DEPS      = RpLibrary.o RpLibraryCInterface.o RpLibraryFInterface.o \
                  scew_extras.o
RP_UNITS_DEPS   = RpUnitsStd.o RpUnits.o RpUnitsCInterface.o RpUnitsFInterface.o
RP_OTHER_DEPS   = RpFortranCommon.o RpBindingsDict.o
RP_OBJS_DEP     = RpVariable.o RpAbout.o RpNumber.o RpString.o RpBoolean.o \
                  RpChoice.o RpOption.o RpUnitsStd.o RpUnits.o

LDLIB_MACOSX = -dynamiclib -o $(LIB_DIR)/$@.dylib
LDLIB_LINUX = -shared -Wl,-rpath,$(LIB_DIR)/ -Wl,-soname,$@.so -o $(LIB_DIR)/$@.so.0.0
# include rappture library definitions

#### librappture shared object ###########################################

librappture: $(RP_IO_DEPS) $(RP_UNITS_DEPS) $(RP_OTHER_DEPS)
	if test "`uname`" == "Darwin"; then \
		$(CXX) $(DEGUG) $(LDLIB_MACOSX) $^ $(LIB_SCEW_FLAG) -lm; \
	else \
		$(CXX) $(DEGUG) $(LDLIB_LINUX) $^ $(LIB_SCEW_FLAG) -lm; \
		/sbin/ldconfig -n $(LIB_DIR); \
	fi

install_rappture: librappture
	if test "`uname`" == "Darwin"; then \
		cp $<.dylib $(RP_INSTALL_BASE)/lib; \
	else \
		cp -d $<.so* $(RP_INSTALL_BASE)/lib; \
	fi

#### libRpObjects ########################################################
libRpObjects: $(RP_OBJS_DEP)
	if test "`uname`" == "Darwin"; then \
		$(CXX) $(DEGUG) $(LDLIB_MACOSX) $^ -lstdc++; \
	else \
		$(CXX) $(DEGUG) $(LDLIB_LINUX) $^ -lstdc++; \
		/sbin/ldconfig -n $(LIB_DIR); \
	fi

install_rpobjs: libRpObjects
	if test "`uname`" == "Darwin"; then \
		cp $<.dylib $(RP_INSTALL_BASE)/lib; \
	else \
		cp -d $<.so* $(RP_INSTALL_BASE)/lib; \
	fi

#### Python ########################################################
Rappture.Units: RpUnits.o RpUnitsStd.o
	$(PY) $(PY_SRC)/setup.py build

install_py: RpUnits.o RpUnitsStd.o
	$(PY) $(PY_SRC)/setup.py install


#### Matlab ########################################################
install_matlab: RpMatlab
	cp -d $(MATLAB_SRC)/*.mex* $(RP_INSTALL_BASE)/lib

#### Octave ########################################################
install_octave: RpOctave
	cp -d $(OCTAVE_SRC)/*.oct* $(RP_INSTALL_BASE)/lib

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


# matlab bindings
MEX_INCLS = $(INCL_MATLAB) $(INCL_CORE) $(INCL_CEE) $(INCL_RP_DEPS)
MATLAB_COMP_ARGS = $(MATLAB_SRC)/RpMatlabInterface.cc $(MEX_INCLS) -L$(LIB_DIR) -lrappture

RpMatlab:
	$(MEX) $(MATLAB_SRC)/rpLib.cc              $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpXml.cc              $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpElement.cc          $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpElementAsObject.cc  $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpElementAsType.cc    $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpElementAsComp.cc    $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpElementAsId.cc      $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpChildren.cc         $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpChildrenByType.cc   $(MATLAB_COMP_ARGS)
#$(MEX) $(MATLAB_SRC)/rpChildrenAsObject.cc    $(MATLAB_COMP_ARGS)
#$(MEX) $(MATLAB_SRC)/rpChildrenAsType.cc      $(MATLAB_COMP_ARGS)
#$(MEX) $(MATLAB_SRC)/rpChildrenAsComp.cc      $(MATLAB_COMP_ARGS)
#$(MEX) $(MATLAB_SRC)/rpChildrenAsId.cc        $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpGet.cc              $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpGet.cc              $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpGetString.cc        $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpGetDouble.cc        $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpPut.cc              $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpPutString.cc        $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpPutStringId.cc      $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpPutDouble.cc        $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpPutDoubleId.cc      $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpNodeComp.cc         $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpNodeType.cc         $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpNodeId.cc           $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpResult.cc           $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpDefineUnit.cc       $(MATLAB_COMP_ARGS)
#$(MEX) $(MATLAB_SRC)/rpDefineConv.cc          $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpGetUnits.cc         $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpGetUnitsName.cc     $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpGetExponent.cc      $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpGetBasis.cc         $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpFind.cc             $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpMakeMetric.cc       $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpConvert.cc          $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpConvertStr.cc       $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpConvertObjStr.cc    $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpConvertDbl.cc       $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpConvertObjDbl.cc    $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpAddPresets.cc       $(MATLAB_COMP_ARGS)

OCT_INCLS = $(INCL_OCTAVE) $(INCL_CORE) $(INCL_RP_DEPS)
OCTAVE_COMP_ARGS = $(OCT_INCLS) -L$(LIB_DIR) -lrappture
RpOctave:
	$(OCT) $(OCTAVE_SRC)/rpAddPresets.cc       $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpChildrenByType.cc   $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpChildren.cc         $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpConvert.cc          $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpConvertDbl.cc       $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpConvertStr.cc       $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpConvertObjDbl.cc    $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpConvertObjStr.cc    $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpDefineUnit.cc       $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpElementAsComp.cc    $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpElementAsId.cc      $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpElementAsObject.cc  $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpElementAsType.cc    $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpElement.cc          $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpFind.cc             $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpGetBasis.cc         $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpGet.cc              $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpGetString.cc        $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpGetDouble.cc        $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpGetExponent.cc      $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpGetUnits.cc         $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpGetUnitsName.cc     $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpLib.cc              $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpMakeMetric.cc       $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpNodeComp.cc         $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpNodeId.cc           $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpNodeType.cc         $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpPut.cc              $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpPutDouble.cc        $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpPutDoubleId.cc      $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpPutDoubleId.cc      $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpPutString.cc        $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpPutStringId.cc      $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpResult.cc           $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpXml.cc              $(OCTAVE_COMP_ARGS)



#### CLEAN UP ############################################################
clean: 
	- rm -f *.o librappture.so* libRp*.so* rp*.mex* $(OCTAVE_SRC)/rp*.o*
	- rm -rf build
