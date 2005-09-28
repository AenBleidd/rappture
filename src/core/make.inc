Rp%.o: $(CORE_SRC)/Rp%.cc 
	$(CPP) -fPIC $(DEBUG) $(INCL_CORE) -o $@ -c $?
