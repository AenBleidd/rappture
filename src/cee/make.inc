
rappture_interface.o: $(CEE_SRC)/rappture_interface.c
	$(CPP) -fPIC $(DEBUG) $(INCL_CEE) $(INCL_PY) -o $@ -c $<

RpUnitsCInterface.o: $(CEE_SRC)/RpUnitsCInterface.cc
	$(CPP) $(CFLAGS) -fPIC $(DEBUG) $(INCL_CORE) $(INCL_CEE) -o $@ -c $?
