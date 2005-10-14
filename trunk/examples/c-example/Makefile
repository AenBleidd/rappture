# define rappture src directory
RP_BASE         = /opt/rappture

PROGS           = plot plotc

# define our compiling environment
# 
CC              = gcc
CXX             = g++
DEBUG           = -g -Wall
DEBUG_PLUS      = -g -DDEBUG

# define our directories
#
INCLUDES_DIR    = $(RP_BASE)/include
INCL_CEE        = -I $(INCLUDES_DIR)/cee
INCL_CORE       = -I $(INCLUDES_DIR)/core
INCL_RP_DEPS    = -I $(RP_BASE)/include

INCL_FILES      = $(INCL_CEE) $(INCL_CORE) $(INCL_RP_DEPS)

LIB_DIR         = $(RP_BASE)/lib
LIB_INC_PREFIX  = -Wl,-rpath,$(LIB_DIR) -L$(LIB_DIR)
LIB_RAPPTURE    = -Wl,-rpath,$(LIB_DIR) -L$(LIB_DIR) -lrappture

all: $(PROGS)

plot: plot.cc
	$(CXX) $(DEBUG) -DDEBUG $(INCL_FILES) -o $@ $< $(LIB_RAPPTURE)

plotc: plotc.c
	$(CC) $(DEBUG) -DDEBUG $(INCL_FILES) -o $@ $< $(LIB_RAPPTURE) -lm

#### CLEAN UP ############################################################
clean:
	rm -f $(PROGS) run*.xml
