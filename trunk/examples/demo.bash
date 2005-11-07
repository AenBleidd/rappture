#!/bin/bash
#
# use this script to run several rappture examples:
#	% ./rundemo
# when you exit from one example, the next one will start and display on your
# screen.
#

exdir=`pwd`
echo "Current directory: $exdir"

dirs="app-fermi/tcl app-fermi/python app-fermi/fortran c-example graph zoo/structure zoo/image zoo/cloud"

echo "Multiple examples will be run in sequence -->"
for i in $dirs
do
	echo $exdir/$i
	cd $exdir/$i
	rappture
done

cd $exdir/3D
rerun 3d_test_run.xml
