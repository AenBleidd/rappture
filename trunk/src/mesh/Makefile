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
		test_grid2d \
		test_grid \
		test_field \
		test_s2 \
		test_rect2 \
		test_serializer

# define our compiling environment
#
CC              = gcc
CXX             = g++
DEBUG           = -g -Wall $(DEBUG_PLUS)
#DEBUG_PLUS      = -DDEBUG
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

RP_MESH_DEPS = 	const.o util.o reader.o serializer.o \
		grid1d.o grid2d.o field.o grid2d_rect.o \
		node2d.o \
		node3d.o mesh.o element.o

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

###############################################
# include mesh objects
#
#dependencies
#
reader.cc: byte_order.h util.h rp_types.h
grid1d.cc: grid1d.h util.h rp_types.h byte_order.h serializable.h
grid2d.cc: grid2d.h grid1d.h util.h rp_types.h byte_order.h serializable.h
grid2d_rect.cc: grid2d_rect.h grid2d.h grid1d.h util.h rp_types.h byte_order.h serializable.h
field.cc: grid1d.h util.h rp_types.h byte_order.h serializable.h
serializer.cc: serializer.h serializable.h grid1d.h grid2d.h grid2d_rect.h util.h field.h
mesh.cc: mesh.h node3d.h element.h util.h serializable.h
node2d.cc: node2d.h util.h serializable.h
node3d.cc: node3d.h util.h serializable.h

grid2d.o: grid2d.cc
	$(CXX) $(CFLAGS) $(DEBUG) -c $?
grid2d_rect.o: grid2d_rect.cc
	$(CXX) $(CFLAGS) $(DEBUG) -c $?
grid1d.o: grid1d.cc 
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
field.o: field.cc
	$(CXX) $(CFLAGS) $(DEBUG) -c $?

### Test programs ##########

endian: endian.o ../librpmesh.so
	$(CXX) $(DEBUG) -o $@ $< $(LIB_RPMESH)
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
test_grid2d: test_grid2d.o ../librpmesh.so 
	$(CXX) $(DEBUG) -o $@ $< $(LIB_RPMESH)
test_rect2: test_rect2.o ../librpmesh.so 
	$(CXX) $(DEBUG) -o $@ $< $(LIB_RPMESH)
test_grid: test_grid.o ../librpmesh.so 
	$(CXX) $(DEBUG) -o $@ $< $(LIB_RPMESH)
test_field: test_field.o ../librpmesh.so 
	$(CXX) $(DEBUG) -o $@ $< $(LIB_RPMESH)
test_serializer: test_serializer.o ../librpmesh.so 
	$(CXX) $(DEBUG) -o $@ $< $(LIB_RPMESH)
test_s2: test_s2.o ../librpmesh.so
	$(CXX) $(DEBUG) -o $@ $< $(LIB_RPMESH)
testg: testg.o ../librpmesh.so
	$(CXX) $(DEBUG) -o $@ $< $(LIB_RPMESH)
testg2: testg2.o ../librpmesh.so
	$(CXX) $(DEBUG) -o $@ $< $(LIB_RPMESH)


#### CLEAN UP ############################################################
clean: 
	- rm -f *.o *.gch librpmesh.a ../librpmesh.so* $(TEST_PROGS)

