# define rappture src directory
RP_BASE = /opt/rappture

# define our compiling environment
# 
CC 			= gcc
CPP			= g++
DEBUG	 		= -g -Wall
DEBUG_PLUS 		= -g -DDEBUG

# define our directories
#
INCLUDES_DIR 	= $(RP_BASE)/include
INCL_CEE	= -I $(INCLUDES_DIR)/cee
INCL_CORE	= -I $(INCLUDES_DIR)/core
INCL_PY		= -I $(INCLUDES_DIR)/python2.4

LIB_DIR		= $(RP_BASE)/lib
LIB_INC_PREFIX	= -Wl,-rpath,$(LIB_DIR) -L$(LIB_DIR)
LIB_RAPPTURE 	= -Wl,-rpath,$(LIB_DIR) -L$(LIB_DIR) -lrappture

plot: plot.cc 
	$(CPP) $(DEBUG) -DDEBUG $(INCL_CEE) $(INCL_CORE) $(INCL_PY) -o $@ $< $(LIB_RAPPTURE)

#### CLEAN UP ############################################################
clean:
	rm -f plot
