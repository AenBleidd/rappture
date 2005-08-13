$(WORK_DIR)/RpUnits.o: $(SRC_DIR)/RpUnits.cc 
	$(CPP) -fPIC $(DEBUG) $(INCL_CORE) -o $@ -c $?

$(WORK_DIR)/RpUnitsStd.o: $(SRC_DIR)/RpUnitsStd.cc
	$(CPP) -fPIC $(DEBUG) $(INCL_CORE) -o $@ -c $?

$(WORK_DIR)/RpDict.o: $(SRC_DIR)/RpDict.cc
	$(CPP) -fPIC $(DEBUG) $(INCL_CORE) -o $@ -c $?

$(WORK_DIR)/RpVariable.o: $(SRC_DIR)/RpVariable.cc
	$(CPP) -fPIC $(DEBUG) $(INCL_CORE) -o $@ -c $?

$(WORK_DIR)/RpNumber.o: $(SRC_DIR)/RpNumber.cc
	$(CPP) -fPIC $(DEBUG) $(INCL_CORE) -o $@ -c $?

$(WORK_DIR)/RpString.o: $(SRC_DIR)/RpString.cc
	$(CPP) -fPIC $(DEBUG) $(INCL_CORE) -o $@ -c $?

$(WORK_DIR)/RpChoice.o: $(SRC_DIR)/RpChoice.cc
	$(CPP) -fPIC $(DEBUG) $(INCL_CORE) -o $@ -c $?

$(WORK_DIR)/RpBoolean.o: $(SRC_DIR)/RpBoolean.cc
	$(CPP) -fPIC $(DEBUG) $(INCL_CORE) -o $@ -c $?

$(WORK_DIR)/RpOption.o: $(SRC_DIR)/RpOption.cc
	$(CPP) -fPIC $(DEBUG) $(INCL_CORE) -o $@ -c $?

$(WORK_DIR)/RpAbout.o: $(SRC_DIR)/RpAbout.cc
	$(CPP) -fPIC $(DEBUG) $(INCL_CORE) -o $@ -c $?
