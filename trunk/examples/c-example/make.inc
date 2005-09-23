plot: plot.cc 
	$(CPP) $(DEBUG) -DDEBUG $(INCL_CEE) $(INCL_PY) -o $@ $< $(LIB_RAPPTUREIO)

