$(WORK_DIR)/RpUnits.o: $(SRC_DIR)/RpUnits.cc 
	$(CPP) -fPIC $(DEBUG) $(INCL_CORE) -o $@ -c $?

$(WORK_DIR)/RpUnitsStd.o: $(SRC_DIR)/RpUnitsStd.cc
	$(CPP) -fPIC $(DEBUG) $(INCL_CORE) -o $@ -c $?

$(WORK_DIR)/RpDict.o: $(SRC_DIR)/RpDict.cc
	$(CPP) -fPIC $(DEBUG) $(INCL_CORE) -o $@ -c $?
