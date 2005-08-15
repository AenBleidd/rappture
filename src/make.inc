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


#### libRpUnits ###########################################################
libRpUnits: $(WORK_DIR)/RpUnits.o $(WORK_DIR)/RpUnitsStd.o
	$(CPP) $(DEGUG) -shared -Wl,-rpath,../lib \
		-Wl,-soname,$@.so -o $(LIB_DIR)/$@.so.0.0 $^ -lc
			    
	/sbin/ldconfig -n $(LIB_DIR)

#### libRpVariable ########################################################
libRpVariable: $(WORK_DIR)/RpVariable.o $(WORK_DIR)/RpAbout.o
	$(CPP) $(DEGUG) -shared -Wl,-rpath,../lib \
		-Wl,-soname,$@.so -o $(LIB_DIR)/$@.so.0.0 $^ -lc
			    
	/sbin/ldconfig -n $(LIB_DIR)

#### libRpNumber ##########################################################
libRpNumber: $(WORK_DIR)/RpNumber.o libRpUnits libRpVariable
	$(CPP) $(DEGUG) -shared -Wl,-rpath,../lib \
		-Wl,-soname,$@.so -o $(LIB_DIR)/$@.so.0.0 $< -lc $(LIB_RP_UNITS) $(LIB_RP_VARIABLE)
			    
	/sbin/ldconfig -n $(LIB_DIR)

#### libRpString ##########################################################
libRpString: $(WORK_DIR)/RpString.o libRpVariable
	$(CPP) $(DEGUG) -shared -Wl,-rpath,../lib \
		-Wl,-soname,$@.so -o $(LIB_DIR)/$@.so.0.0 $< -lc $(LIB_RP_VARIABLE)
			    
	/sbin/ldconfig -n $(LIB_DIR)

#### libRpBoolean ##########################################################
libRpBoolean: $(WORK_DIR)/RpBoolean.o libRpVariable
	$(CPP) $(DEGUG) -shared -Wl,-rpath,../lib \
		-Wl,-soname,$@.so -o $(LIB_DIR)/$@.so.0.0 $< -lc $(LIB_RP_VARIABLE)
			    
	/sbin/ldconfig -n $(LIB_DIR)

#### libRpChoice ##########################################################
libRpChoice: $(WORK_DIR)/RpChoice.o libRpVariable libRpOption
	$(CPP) $(DEGUG) -shared -Wl,-rpath,../lib \
		-Wl,-soname,$@.so -o $(LIB_DIR)/$@.so.0.0 $< -lc $(LIB_RP_VARIABLE) $(LIB_RP_OPTION)
			    
	/sbin/ldconfig -n $(LIB_DIR)

#### libRpOption ##########################################################
libRpOption: $(WORK_DIR)/RpOption.o $(WORK_DIR)/RpAbout.o 
	$(CPP) $(DEGUG) -shared -Wl,-rpath,../lib \
		-Wl,-soname,$@.so -o $(LIB_DIR)/$@.so.0.0 $< -lc $(LIB_RP_ABOUT)
			    
	/sbin/ldconfig -n $(LIB_DIR)

