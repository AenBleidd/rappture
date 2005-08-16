
SRC_TEST            = $(TEST_DIR)/src

TEST_PROGS          = RpBoolean_test    \
                      RpChoice_test     \
                      RpNumber_test     \
                      RpString_test     \
                      RpUnits_test      \
                      RpVariable_test   \
                      RpUnitsC_test

test: $(TEST_PROGS)

RpBoolean_test: $(SRC_TEST)/RpBoolean_test.cc libRpBoolean
	$(CPP) $(DEBUG) $(INCL_CORE) -o $(BIN_DIR)/$@ $< $(LIB_RP_BOOLEAN)
    
RpChoice_test: $(SRC_TEST)/RpChoice_test.cc libRpChoice
	$(CPP) $(DEBUG) $(INCL_CORE) -o $(BIN_DIR)/$@ $< $(LIB_RP_CHOICE)
    
RpNumber_test: $(SRC_TEST)/RpNumber_test.cc libRpNumber
	$(CPP) $(DEBUG) $(INCL_CORE) -o $(BIN_DIR)/$@ $< $(LIB_RP_NUMBER)

RpString_test: $(SRC_TEST)/RpString_test.cc libRpString
	$(CPP) $(DEBUG) $(INCL_CORE) -o $(BIN_DIR)/$@ $< $(LIB_RP_STRING)

RpUnits_test: $(SRC_TEST)/RpUnits_test.cc libRpUnits
	$(CPP) $(DEBUG) $(INCL_CORE) -o $(BIN_DIR)/$@ $< $(LIB_RP_UNITS)

RpVariable_test: $(SRC_TEST)/RpVariable_test.cc libRpVariable
	$(CPP) $(DEBUG) $(INCL_CORE) -o $(BIN_DIR)/$@ $< $(LIB_RP_VARIABLE)

RpUnitsC_test: $(SRC_TEST)/RpUnitsC_test.c $(WORK_DIR)/RpUnitsCInterface.o
	$(CC) $(DEBUG) $(INCL_CEE) $(INCL_CORE) -o $(BIN_DIR)/$@ $^ $(LIB_RP_UNITS)

