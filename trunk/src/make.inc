#### librappture shared object ###########################################

libRpUnits_CInterface: RpUnitsCInterface.o RpUnitsStd.o RpUnits.o
	$(CPP) $(DEGUG) -shared -Wl,-rpath,./ \
		-Wl,-soname,$@.so -o $@.so.0.0 $^ -lc
	
	/sbin/ldconfig -n ./

libRpUnits_FInterface: RpUnitsStd.o RpUnits.o RpUnits_fortran.o 
	$(CPP) $(DEBUG) -shared -Wl,-rpath,./ \
        -Wl,-soname,$@.so -o $@.so.0.0 $^ -lstdc++
	
	/sbin/ldconfig -n ./

librappture: rappture_fortran.o rappture_interface.o
	$(CC) $(DEGUG) -shared -Wl,-rpath,./ \
		-Wl,-soname,$@.so -o $@.so.0.0 $^ $(EMB_PY_FLAGS) -lstdc++
		
	/sbin/ldconfig -n ./
    
libRapptureIO: rappture_fortran.o rappture_interface.o
	$(CC) $(DEGUG) -shared -Wl,-rpath,./ \
		-Wl,-soname,$@.so -o $@.so.0.0 $^ $(EMB_PY_FLAGS) -lstdc++ 
		
	/sbin/ldconfig -n ./


#### libRpUnits ###########################################################
libRpUnits: RpUnits.o RpUnitsStd.o
	$(CPP) $(DEGUG) -shared -Wl,-rpath,./ \
		-Wl,-soname,$@.so -o $@.so.0.0 $^ -lc
			    
	/sbin/ldconfig -n ./

#### libRpVariable ########################################################
libRpVariable: RpVariable.o RpAbout.o
	$(CPP) $(DEGUG) -shared -Wl,-rpath,./ \
		-Wl,-soname,$@.so -o $@.so.0.0 $^ -lc
			    
	/sbin/ldconfig -n ./

#### libRpNumber ##########################################################
libRpNumber: RpNumber.o libRpUnits libRpVariable
	$(CPP) $(DEGUG) -shared -Wl,-rpath,./ \
		-Wl,-soname,$@.so -o $@.so.0.0 $< -lc $(LIB_RP_UNITS) $(LIB_RP_VARIABLE)
			    
	/sbin/ldconfig -n ./

#### libRpString ##########################################################
libRpString: RpString.o libRpVariable
	$(CPP) $(DEGUG) -shared -Wl,-rpath,./ \
		-Wl,-soname,$@.so -o $@.so.0.0 $< -lc $(LIB_RP_VARIABLE)
			    
	/sbin/ldconfig -n ./

#### libRpBoolean ##########################################################
libRpBoolean: RpBoolean.o libRpVariable
	$(CPP) $(DEGUG) -shared -Wl,-rpath,./ \
		-Wl,-soname,$@.so -o $@.so.0.0 $< -lc $(LIB_RP_VARIABLE)
			    
	/sbin/ldconfig -n ./

#### libRpChoice ##########################################################
libRpChoice: RpChoice.o libRpVariable libRpOption
	$(CPP) $(DEGUG) -shared -Wl,-rpath,./ \
		-Wl,-soname,$@.so -o $@.so.0.0 $< -lc $(LIB_RP_VARIABLE) $(LIB_RP_OPTION)
			    
	/sbin/ldconfig -n ./

#### libRpOption ##########################################################
libRpOption: RpOption.o RpAbout.o 
	$(CPP) $(DEGUG) -shared -Wl,-rpath,./ \
		-Wl,-soname,$@.so -o $@.so.0.0 $< -lc $(LIB_RP_ABOUT)
			    
	/sbin/ldconfig -n ./

