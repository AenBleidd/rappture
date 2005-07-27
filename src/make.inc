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
