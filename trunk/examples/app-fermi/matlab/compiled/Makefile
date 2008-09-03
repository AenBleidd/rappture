APPNAME = fermi

${APPNAME}: fermi_main.m fermi.m
	mcc -m -R -nojvm -I /apps/rappture/lib/matlab -o $@ $^

all: ${APPNAME}

install: ${APPNAME}
	sed s/YOURAPPNAME/$^/ < run_app.patch | patch run_$^.sh
	chmod +x run_$^.sh
#	mv $^ $^.ctf run_$^.sh ../bin
#	mv readme.txt ../bin/$^.readme
#	LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/apps/rappture/lib `pwd`/../bin/run_$^.sh /apps/matlab/mcr `pwd`/driver.xml `pwd`/../data
	simsim --tool tool.xml --driver-only driver.${APPNAME}
	LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/apps/rappture/lib `pwd`/run_$^.sh /apps/matlab/mcr `pwd`/driver.${APPNAME}
	rm -f run*.xml driver.${APPNAME}
	@echo "Install Successful"

clean:
	rm -rf ${APPNAME} \
		${APPNAME}_main.c \
		${APPNAME}.ctf \
		${APPNAME}.prj \
		${APPNAME}_mcc_component_data.c \
		run_${APPNAME}.sh \
		mccExcludedFiles.log \
		readme.txt \
		${APPNAME}_mcr

distclean: clean
#	rm -rf ../bin/*
