#
# Replace this with the location of your rappture installation:
RAPPTURE_DIR = /opt/rappture

PROGS			= 	fermi_example

# define our compiling environment
# 
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


LIB_DIR = $(RAPPTURE_DIR)/lib
LIB_RAPPTURE 	= -Wl,-rpath,$(LIB_DIR) -L$(LIB_DIR) -lrappture

all: ${PROGS}

#### fermi_example ########################################################

fermi_example: fermi_example.f
	$(F77) -g -Wall $(LIB_RAPPTURE) -o $@ $<

mac: fermi_example.f
	$(F77) -g -Wall -L$(LIB_DIR) -lrappture -lscew  -o fermi_example $<

#### CLEAN UP ############################################################
clean: 
	- rm -f ${PROGS} run*.xml
