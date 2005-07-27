
$(WORK_DIR)/RpUnits_fortran.o: $(FORT_SRC)/RpUnits_fortran.c
	$(CPP) $(CFLAGS) -fPIC $(DEBUG) $(INCL_CORE) -o $@ -c $?

$(WORK_DIR)/rappture_fortran.o: $(FORT_SRC)/rappture_fortran.c
	$(CPP) $(CFLAGS) -fPIC $(DEBUG) $(INCL_CORE) $(INCL_CEE) $(INCL_PY) -o $@ -c $<
