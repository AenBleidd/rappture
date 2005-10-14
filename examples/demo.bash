#!/bin/bash
#
# use this script to run several rappture examples:
#	% ./rundemo
# when you exit from one example, the next one will start and display on your
# screen.
#

exdir=/opt/rappture/examples
dirs="app-fermi/tcl app-fermi/python app-fermi/fortran c-example graph zoo/structure"

echo "7 examples will be shown-->"
for i in $dirs
do
	echo $exdir/$i
	cd $exdir/$i
	/opt/rappture/bin/rappture
done

cd $exdir/3D
/opt/rappture/bin/rerun 3d_test_run.xml
