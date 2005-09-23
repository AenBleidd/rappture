# define the top of our directory structure
# replace this with the full path of the directory
# containing the rappture directory
TOP_DIR         = ../../..

# define the top of the rappture directory structure
RP_BASE   = $(TOP_DIR)/rappture

# define which programs can be made
#PROGS			= 	libRpUnits_CInterface 	\
#					libRpUnits_FInterface 	\
#					librappture				\
#					libRapptureIO			\
#					libRpUnits				\
#					libRpVariable			\
#					libRpNumber				\
#					libRpString				\
#					libRpBoolean			\
#					libRpChoice				\
#					libRpOption				
PROGS = plot
					

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

F77		        = g77
#F77				= f77
#F77				= ifort
#F77				= /opt/mpich-1.2.6/p4-intel/bin/mpif90

LN              = ln

# define our directories
#
WORK_DIR 		= $(RP_BASE)/work
INCLUDES_DIR 	= $(RP_BASE)/include
BIN_DIR			= $(RP_BASE)/bin
LIB_DIR			= $(RP_BASE)/lib
SRC_DIR			= $(RP_BASE)/src
TEST_DIR		= $(RP_BASE)/test

CORE_SRC 		= $(SRC_DIR)/core
FORT_SRC		= $(SRC_DIR)/fortran
CEE_SRC			= $(SRC_DIR)/cee
PY_SRC			= $(SRC_DIR)/python

#LIB_RPUNITS 	= -Wl,-rpath,$(LIB_DIR) -L$(LIB_DIR) -lRpUnits
#LIB_RPDICT 		= -Wl,-rpath,$(LIB_DIR) -L$(LIB_DIR) -lRpDict

LIB_INC_PREFIX	= -Wl,-rpath,$(LIB_DIR) -L$(LIB_DIR)

#LIB_RPUNITS_C   = $(LIB_INC_PREFIX) -lRpUnits_CInterface
#LIB_RAPPTUREIO  = $(LIB_INC_PREFIX) -lrappture

LIB_RP_UNITS 	= $(LIB_INC_PREFIX) -lRpUnits
LIB_RP_VARIABLE = $(LIB_INC_PREFIX) -lRpVariable
LIB_RP_NUMBER 	= $(LIB_INC_PREFIX) -lRpNumber
LIB_RP_STRING 	= $(LIB_INC_PREFIX) -lRpString
LIB_RP_BOOLEAN 	= $(LIB_INC_PREFIX) -lRpBoolean
LIB_RP_OPTION 	= $(LIB_INC_PREFIX) -lRpOption
LIB_RP_CHOICE 	= $(LIB_INC_PREFIX) -lRpChoice
LIB_RP_PRESET 	= $(LIB_INC_PREFIX) -lRpPreset


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
#vpath %.c $(CORE_SRC)
#vpath %.cc $(CORE_SRC)
#vpath %.tcc $(CORE_SRC)
#vpath %.cpp $(CORE_SRC)


# default: 

all: ${PROGS} plot

# include rappture library definitions
#include $(SRC_DIR)/make.inc

# include core source files
#include $(CORE_SRC)/make.inc

# include cee binding definitions
#include $(CEE_SRC)/make.inc

# include fortran binding definitions
#include $(FORT_SRC)/make.inc

# include test programs
#include $(TEST_DIR)/make.inc

# include new example
include $(RP_BASE)/examples/c-example/make.inc

#### CLEAN UP ############################################################
clean:
	rm -f plot
#clean: 
#	- rm -f $(BIN_DIR)/* $(WORK_DIR)/*.o $(LIB_DIR)/librappture.* $(LIB_DIR)/libR*.so* 
