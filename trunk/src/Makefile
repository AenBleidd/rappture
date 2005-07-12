# define which programs this effects
PROGS			= 	libRpUnits_CInterface 	\
					libRpUnits_FInterface 	\
					librappture				\
					libRapptureIO			\

# define our compiling environment
# 
AR				= ar
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

 F77			= g77
#F77				= f77
#F77				= ifort
#F77				= /opt/mpich-1.2.6/p4-intel/bin/mpif90

LN              = ln

# define our directories
#
WORK_DIR 		= ../work
INCLUDES_DIR 	= ../include
SRC_DIR 		= core
FORT_SRC		= fortran
CEE_SRC			= cee
PY_SRC			= python
BIN_DIR			= ../bin
LIB_DIR			= ../lib

#LIB_RPUNITS 	= -Wl,-rpath,$(LIB_DIR) -L$(LIB_DIR) -lRpUnits
#LIB_RPDICT 		= -Wl,-rpath,$(LIB_DIR) -L$(LIB_DIR) -lRpDict

LIB_RPUNITS_C 	= -Wl,-rpath,$(LIB_DIR) -L$(LIB_DIR) -lRpUnits_CInterface
LIB_RPUNITS_F 	= -Wl,-rpath,$(LIB_DIR) -L$(LIB_DIR) -lRpUnits_FInterface
LIB_RAPPTURE 	= -Wl,-rpath,$(LIB_DIR) -L$(LIB_DIR) -lrappture
LIB_RAPPTUREIO	= -Wl,-rpath,$(LIB_DIR) -L$(LIB_DIR) -lRapptureIO

INCL_CORE 		= -I $(INCLUDES_DIR)/core
INCL_CEE		= -I $(INCLUDES_DIR)/cee
INCL_FORTRAN	= -I $(INCLUDES_DIR)/fortran
INCL_PY			= -I $(INCLUDES_DIR)/python

# you need to change this to where your version of python is installed.
EMB_PY_FLAGS    = -L$(LIB_DIR) -lpython2.4

# tell make where to find files of each specific type
#
vpath %.o $(WORK_DIR)
vpath %.so $(LIB_DIR)
vpath %.h $(INCLUDES_DIR)
#vpath %.c $(SRC_DIR)
#vpath %.cc $(SRC_DIR)
#vpath %.tcc $(SRC_DIR)
#vpath %.cpp $(SRC_DIR)


# default: 

all: ${PROGS}

#### librappture shared object ###########################################

libRpUnits_CInterface: $(WORK_DIR)/RpUnitsCInterface.o $(WORK_DIR)/RpUnitsStd.o $(WORK_DIR)/RpUnits.o
	$(CPP) $(DEGUG) -shared -Wl,-rpath,../lib \
		-Wl,-soname,$@.so -o $(LIB_DIR)/$@.so.0.0 $^ -lc
	
	/sbin/ldconfig -n $(LIB_DIR)

libRpUnits_FInterface: $(WORK_DIR)/RpUnitsStd.o $(WORK_DIR)/RpUnits.o $(WORK_DIR)/RpUnits_fortran.o 
	$(CPP) $(DEBUG) -shared -Wl,-rpath,../lib \
        -Wl,-soname,$@.so -o $(LIB_DIR)/$@.so.0.0 $^ -lstdc++
	
	/sbin/ldconfig -n $(LIB_DIR)

librappture: $(WORK_DIR)/rappture_fortran.o $(WORK_DIR)/rappture_interface.o
	$(CC) $(DEGUG) -shared -Wl,-rpath,../lib \
		-Wl,-soname,$@.so -o $(LIB_DIR)/$@.so.0.0 $^ $(EMB_PY_FLAGS) -lstdc++
		
	/sbin/ldconfig -n $(LIB_DIR)
    
libRapptureIO: $(WORK_DIR)/rappture_fortran.o $(WORK_DIR)/rappture_interface.o
	$(CC) $(DEGUG) -shared -Wl,-rpath,../lib \
		-Wl,-soname,$@.so -o $(LIB_DIR)/$@.so.0.0 $^ $(EMB_PY_FLAGS) -lstdc++ 
		
	/sbin/ldconfig -n $(LIB_DIR)
    

# -fPIC is for Platform Independent Code, used when creating shared objects.

$(WORK_DIR)/RpUnits.o: $(SRC_DIR)/RpUnits.cc 
	$(CPP) -fPIC $(DEBUG) $(INCL_CORE) -o $@ -c $?

$(WORK_DIR)/RpUnitsStd.o: $(SRC_DIR)/RpUnitsStd.cc
	$(CPP) -fPIC $(DEBUG) $(INCL_CORE) -o $@ -c $?

$(WORK_DIR)/RpDict.o: $(SRC_DIR)/RpDict.cc
	$(CPP) -fPIC $(DEBUG) $(INCL_CORE) -o $@ -c $?

$(WORK_DIR)/rappture_interface.o: $(CEE_SRC)/rappture_interface.c
	$(CPP) -fPIC $(DEBUG) $(INCL_CEE) -o $(WORK_DIR)/$@ -c $<

$(WORK_DIR)/RpUnitsCInterface.o: $(CEE_SRC)/RpUnitsCInterface.cc
	$(CPP) $(CFLAGS) -fPIC $(DEBUG) $(INCL_CORE) $(INCL_CEE)  -o $@ -c $?

$(WORK_DIR)/RpUnits_fortran.o: $(FORT_SRC)/RpUnits_fortran.c
	$(CPP) $(CFLAGS) -fPIC $(DEBUG) $(INCL_CORE) -o $@ -c $?

$(WORK_DIR)/rappture_fortran.o: $(FORT_SRC)/rappture_fortran.c
	$(CPP) $(CFLAGS) -fPIC $(DEBUG) $(INCL_CORE) $(INCL_CEE) -o $(WORK_DIR)/$@ -c $<


#### CLEAN UP ############################################################
clean: 
	- rm -f $(BIN_DIR)/* $(WORK_DIR)/*.o
