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
TOP_DIR         = /home/cxsong

# define the top of the rappture directory structure
RP_BASE   = $(TOP_DIR)/rappture

# define which programs can be made
PROGS           =   librpmesh
TEST_PROGS =	test_node2d \
		test_node3d \
		test_element \
		test_elem2 \
		test_mesh \
		test_grid1d \
		test_grid

# define our compiling environment
#
CC              = gcc
CXX             = g++
DEBUG           = -g -Wall $(DEBUG_PLUS)
DEBUG_PLUS      = -DDEBUG
PY              = $(RP_INSTALL_BASE)/bin/python
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

MESH_SRC        = $(SRC_DIR)/mesh
CORE_SRC        = $(SRC_DIR)/core
FORT_SRC        = $(SRC_DIR)/fortran
CEE_SRC         = $(SRC_DIR)/cee
PY_SRC          = $(SRC_DIR)/python
MATLAB_SRC      = $(SRC_DIR)/matlab
OCTAVE_SRC      = $(SRC_DIR)/octave

LIB_INC_PREFIX  = -Wl,-rpath,$(LIB_DIR) -L$(LIB_DIR)
LIB_RP_OBJECTS  = $(LIB_INC_PREFIX) -lRpObjects
LIB_RAPPTURE    = -Wl,-rpath,$(LIB_DIR) -L$(LIB_DIR) -lrappture
LIB_RPMESH      = -Wl,-rpath,$(LIB_DIR) -L/$(LIB_DIR) -lrpmesh

INCL_CORE       = -I$(INCLUDES_DIR)/core
INCL_CEE        = -I$(INCLUDES_DIR)/cee
INCL_FORTRAN    = -I$(INCLUDES_DIR)/fortran
INCL_PY         = -I$(INCLUDES_DIR)/python
INCL_MATLAB     = -I$(INCLUDES_DIR)/matlab
INCL_OCTAVE     = -I$(INCLUDES_DIR)/octave

CFLAGS = -fPIC
CCFLAGS = -fPIC -g

# default:

all: ${PROGS} 

test: ${TEST_PROGS}

#install: install_py install_rappture install_rpobjs install_matlab install_octave

RP_IO_DEPS      = RpLibrary.o RpLibraryCInterface.o RpLibraryFInterface.o \
                  scew_extras.o
RP_UNITS_DEPS   = RpUnitsStd.o RpUnits.o RpUnitsCInterface.o RpUnitsFInterface.o
RP_OTHER_DEPS   = RpFortranCommon.o RpBindingsDict.o
RP_OBJS_DEP     = RpVariable.o RpAbout.o RpNumber.o RpString.o RpBoolean.o \
                  RpChoice.o RpOption.o RpUnitsStd.o RpUnits.o #RpValue.o\
                  # RpValueDouble.o RpValueString.o
RP_MESH_DEPS = 	const.o grid1d_reg.o grid2d.o node3d.o node2d.o \
		element.o mesh.o serializer.o util.o \
		grid1d.o reader.o #field.o 

LDLIB_MACOSX = -dynamiclib -o $(LIB_DIR)/$@.dylib
LDLIB_LINUX = -shared -Wl,-rpath,$(LIB_DIR)/ -Wl,-soname,$@.so -o $(LIB_DIR)/$@.so.0.0
LDLIB_LINUX_MESH = -shared -Wl,-rpath,$(LIB_DIR)/mesh/ -Wl,-soname,$@.so -o $(LIB_DIR)/mesh/$@.so.0.0

# include rappture library definitions

#### librappture shared object ###########################################

librpmesh: $(RP_MESH_DEPS)
	if test "`uname`" == "Darwin"; then \
		$(CXX) $(DEGUG) $(LDLIB_MACOSX) $^ $(LIB_SCEW_FLAG) -lm; \
		ar -r $@.a $^; \
		ranlib -s $@.a; \
	else \
		$(CXX) $(DEGUG) $(LDLIB_LINUX) $^; \
		/sbin/ldconfig -n $(LIB_DIR); \
		ar -r $@.a $^; \
		ranlib $@.a; \
	fi
librappture: $(RP_IO_DEPS) $(RP_UNITS_DEPS) $(RP_OTHER_DEPS)
	if test "`uname`" == "Darwin"; then \
		$(CXX) $(DEGUG) $(LDLIB_MACOSX) $^ $(LIB_SCEW_FLAG) -lm; \
		ar -r $@.a $^; \
		ranlib -s $@.a; \
	else \
		$(CXX) $(DEGUG) $(LDLIB_LINUX) $^ $(LIB_SCEW_FLAG) -lm; \
		/sbin/ldconfig -n $(LIB_DIR); \
		ar -r $@.a $^; \
		ranlib $@.a; \
	fi

install_rappture: librappture
	if test "`uname`" == "Darwin"; then \
		cp $<.dylib $(RP_INSTALL_BASE)/lib; \
		cp $<.a $(RP_INSTALL_BASE)/lib; \
	else \
		cp -d $<.so* $(RP_INSTALL_BASE)/lib; \
		cp -d $<.a $(RP_INSTALL_BASE)/lib; \
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
install_matlab: librappture RpMatlab
	if test ! -d $(RP_INSTALL_BASE)/lib/matlab; then \
		mkdir $(RP_INSTALL_BASE)/lib/matlab; \
	fi
	cp -d $(MATLAB_SRC)/*.mex* $(RP_INSTALL_BASE)/lib/matlab

#### Octave ########################################################
install_octave: librappture RpOctave
	if test ! -d $(RP_INSTALL_BASE)/lib/octave; then \
		mkdir $(RP_INSTALL_BASE)/lib/octave; \
	fi
	cp -d $(OCTAVE_SRC)/*.oct* $(RP_INSTALL_BASE)/lib/octave


###############################################
# include mesh objects
#
#dependencies
#
grid1d.cc: grid1d.h util.h rp_types.h byte_order.h serializable.h
grid2d.cc: grid1d.h util.h rp_types.h byte_order.h serializable.h
mesh.cc: mesh.h node3d.h element.h util.h serializable.h
node2d.cc: node2d.h util.h serializable.h
node3d.cc: node3d.h util.h serializable.h
#serializable.cc: serializable.h util.h rp_types.h byte_order.h 
serializer.cc: serializer.h serializable.h
reader.cc: byte_order.h

grid2d.o: grid2d.cc
	$(CXX) $(CFLAGS) $(DEBUG) -c $?
grid1d.o: grid1d.cc 
	$(CXX) $(CFLAGS) $(DEBUG) -c $?
grid1d_reg.o: grid1d_reg.cc
	$(CXX) $(CFLAGS) $(DEBUG) -c $?
util.o: util.cc
	$(CXX) $(CFLAGS) $(DEBUG) -c $?
const.o: const.cc
	$(CXX) $(CFLAGS) $(DEBUG) -c $?
node2d.o: node2d.cc
	$(CXX) $(CFLAGS) $(DEBUG) -c $?
node3d.o: node3d.cc
	$(CXX) $(CFLAGS) $(DEBUG) -c $?
element.o: element.cc
	$(CXX) $(CFLAGS) $(DEBUG) -c $?
mesh.o: mesh.cc
	$(CXX) $(CFLAGS) $(DEBUG) -c $?
serializer.o: serializer.cc
	$(CXX) $(CFLAGS) $(DEBUG) -c $?
reader.o: reader.cc
	$(CXX) $(CFLAGS) $(DEBUG) -c $?

# include core source files

RpLibrary.o: $(CORE_SRC)/RpLibrary.cc
	$(CXX) $(CFLAGS) $(DEBUG) $(INCL_CORE) $(INCL_RP_DEPS) -o $@ -c $?

scew_extras.o: $(CORE_SRC)/scew_extras.c
	$(CC) $(CFLAGS) $(DEBUG) $(INCL_CORE) $(INCL_RP_DEPS) -o $@ -c $?

RpVariable.o: $(CORE_SRC)/RpVariable.cc
	$(CXX) $(CFLAGS) $(DEBUG) $(INCL_CORE) -o $@ -c $?

RpAbout.o: $(CORE_SRC)/RpAbout.cc
	$(CXX) $(CFLAGS) $(DEBUG) $(INCL_CORE) -o $@ -c $?

RpNumber.o: $(CORE_SRC)/RpNumber.cc
	$(CXX) $(CFLAGS) $(DEBUG) $(INCL_CORE) -o $@ -c $?

RpString.o: $(CORE_SRC)/RpString.cc
	$(CXX) $(CFLAGS) $(DEBUG) $(INCL_CORE) -o $@ -c $?

RpBoolean.o: $(CORE_SRC)/RpBoolean.cc
	$(CXX) $(CFLAGS) $(DEBUG) $(INCL_CORE) -o $@ -c $?

RpChoice.o: $(CORE_SRC)/RpChoice.cc
	$(CXX) $(CFLAGS) $(DEBUG) $(INCL_CORE) -o $@ -c $?

RpOption.o: $(CORE_SRC)/RpOption.cc
	$(CXX) $(CFLAGS) $(DEBUG) $(INCL_CORE) -o $@ -c $?

RpUnitsStd.o: $(CORE_SRC)/RpUnitsStd.cc
	$(CXX) $(CFLAGS) $(DEBUG) $(INCL_CORE) -o $@ -c $?

RpUnits.o: $(CORE_SRC)/RpUnits.cc
	$(CXX) $(CFLAGS) $(DEBUG) $(INCL_CORE) -o $@ -c $?

RpBindingsDict.o: $(CORE_SRC)/RpBindingsDict.cc
	$(CXX) $(CFLAGS) $(DEBUG) $(INCL_CORE) -o $@ -c $?

RpValue.o: $(CORE_SRC)/RpValue.cc
	$(CXX) $(CFLAGS) $(DEBUG) $(INCL_CORE) -o $@ -c $?

RpValueDouble.o: $(CORE_SRC)/RpValueDouble.cc
	$(CXX) $(CFLAGS) $(DEBUG) $(INCL_CORE) -o $@ -c $?

RpValueString.o: $(CORE_SRC)/RpValueString.cc
	$(CXX) $(CFLAGS) $(DEBUG) $(INCL_CORE) -o $@ -c $?



# include cee binding definitions

RpUnitsCInterface.o: $(CEE_SRC)/RpUnitsCInterface.cc
	$(CXX) $(CFLAGS) $(DEBUG) $(INCL_CORE) $(INCL_CEE) -o $@ -c $?

RpLibraryCInterface.o: $(CEE_SRC)/RpLibraryCInterface.cc
	$(CXX) $(CFLAGS) $(DEBUG) $(INCL_CORE) $(INCL_CEE) $(INCL_RP_DEPS) -o $@ -c $?




# include fortran binding definitions

RpUnitsFInterface.o: $(FORT_SRC)/RpUnitsFInterface.cc
	$(CXX) $(CFLAGS) $(DEBUG) $(INCL_CORE) $(INCL_FORTRAN) -o $@ -c $?

RpFortranCommon.o: $(FORT_SRC)/RpFortranCommon.c
	$(CXX) $(CFLAGS) $(DEBUG) $(INCL_FORTRAN) -o $@ -c $<

RpLibraryFInterface.o: $(FORT_SRC)/RpLibraryFInterface.cc
	$(CXX) $(CFLAGS) $(DEBUG) $(INCL_CORE) $(INCL_FORTRAN) $(INCL_RP_DEPS) -o $@ -c $?


# matlab bindings
MEX_INCLS = $(INCL_MATLAB) $(INCL_CORE) $(INCL_CEE) $(INCL_RP_DEPS)
MEX_OPTS = -g -outdir $(MATLAB_SRC) -L$(LIB_DIR) -lrappture
MATLAB_COMP_ARGS = $(MATLAB_SRC)/RpMatlabInterface.cc $(MEX_INCLS) $(MEX_OPTS)

RpMatlab:
	$(MEX) $(MATLAB_SRC)/rpLib.cc                   $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpLibXml.cc                $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpLibElement.cc            $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpLibElementAsObject.cc    $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpLibElementAsType.cc      $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpLibElementAsComp.cc      $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpLibElementAsId.cc        $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpLibChildren.cc           $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpLibChildrenByType.cc     $(MATLAB_COMP_ARGS)
#$(MEX) $(MATLAB_SRC)/rpChildrenAsObject.cc         $(MATLAB_COMP_ARGS)
#$(MEX) $(MATLAB_SRC)/rpChildrenAsType.cc           $(MATLAB_COMP_ARGS)
#$(MEX) $(MATLAB_SRC)/rpChildrenAsComp.cc           $(MATLAB_COMP_ARGS)
#$(MEX) $(MATLAB_SRC)/rpChildrenAsId.cc             $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpLibGet.cc                $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpLibGetString.cc          $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpLibGetDouble.cc          $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpLibPut.cc                $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpLibPutString.cc          $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpLibPutDouble.cc          $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpLibNodeComp.cc           $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpLibNodeType.cc           $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpLibNodeId.cc             $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpLibResult.cc             $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpUnitsDefineUnit.cc       $(MATLAB_COMP_ARGS)
#$(MEX) $(MATLAB_SRC)/rpDefineConv.cc               $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpUnitsGetUnits.cc         $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpUnitsGetUnitsName.cc     $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpUnitsGetExponent.cc      $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpUnitsGetBasis.cc         $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpUnitsFind.cc             $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpUnitsMakeMetric.cc       $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpUnitsConvert.cc          $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpUnitsConvertStr.cc       $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpUnitsConvertObjStr.cc    $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpUnitsConvertDbl.cc       $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpUnitsConvertObjDbl.cc    $(MATLAB_COMP_ARGS)
	$(MEX) $(MATLAB_SRC)/rpAddPresets.cc            $(MATLAB_COMP_ARGS)

OCT_INCLS = $(INCL_OCTAVE) $(INCL_CORE) $(INCL_RP_DEPS)
OCTAVE_COMP_ARGS = $(OCT_INCLS) -L$(LIB_DIR) -lrappture
RpOctave:
	$(OCT) $(OCTAVE_SRC)/rpAddPresets.cc            $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpLib.cc                   $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpLibChildrenByType.cc     $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpLibChildren.cc           $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpLibElementAsComp.cc      $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpLibElementAsId.cc        $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpLibElementAsObject.cc    $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpLibElementAsType.cc      $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpLibElement.cc            $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpLibGet.cc                $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpLibGetString.cc          $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpLibGetDouble.cc          $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpLibNodeComp.cc           $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpLibNodeId.cc             $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpLibNodeType.cc           $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpLibPut.cc                $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpLibPutDouble.cc          $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpLibPutString.cc          $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpLibResult.cc             $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpLibXml.cc                $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpUnitsConvert.cc          $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpUnitsConvertDbl.cc       $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpUnitsConvertStr.cc       $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpUnitsConvertObjDbl.cc    $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpUnitsConvertObjStr.cc    $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpUnitsDefineUnit.cc       $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpUnitsFind.cc             $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpUnitsGetBasis.cc         $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpUnitsGetExponent.cc      $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpUnitsGetUnits.cc         $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpUnitsGetUnitsName.cc     $(OCTAVE_COMP_ARGS)
	$(OCT) $(OCTAVE_SRC)/rpUnitsMakeMetric.cc       $(OCTAVE_COMP_ARGS)

docs:
	if test ! -d docs; then \
		mkdir docs; \
	fi
	if test ! -d docs/doxygen; then \
		mkdir docs/doxygen; \
	fi
	doxygen

#### CLEAN UP ############################################################
clean: 
	- rm -f *.o librpmesh.a ../librpmesh.so* $(TEST_PROGS)

#################

endiantest: endiantest.o 
	$(CXX) $(DEBUG) -o $@ $<
test_node3d: test_node3d.o 
	$(CXX) $(DEBUG) -o $@ $< $(LIB_RPMESH)
test_node2d: test_node2d.o 
	$(CXX) $(DEBUG) -o $@ $< $(LIB_RPMESH)
test_element: test_element.o ../librpmesh.so
	$(CXX) $(DEBUG) -o $@ $< $(LIB_RPMESH)
test_elem2: test_elem2.o ../librpmesh.so
	$(CXX) $(DEBUG) -o $@ $< $(LIB_RPMESH)
test_mesh: test_mesh.o ../librpmesh.so
	$(CXX) $(DEBUG) -o $@ $< $(LIB_RPMESH)
test_mesh2: test_mesh2.o ../librpmesh.so
	$(CXX) $(DEBUG) -o $@ $< $(LIB_RPMESH)
test_grid1d: test_grid1d.o ../librpmesh.so 
	$(CXX) $(DEBUG) -o $@ $< $(LIB_RPMESH)
test_grid: test_grid.o ../librpmesh.so 
	$(CXX) $(DEBUG) -o $@ $< $(LIB_RPMESH)
testg: testg.o ../librpmesh.so
	$(CXX) $(DEBUG) -o $@ $< $(LIB_RPMESH)
endian: endian.o ../librpmesh.so
	$(CXX) $(DEBUG) -o $@ $< $(LIB_RPMESH)
trystr: trystr.o 
	$(CXX) $(DEBUG) -o $@ $< 
