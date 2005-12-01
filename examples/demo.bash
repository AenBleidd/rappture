#!/bin/bash
#
# use this script to run several rappture examples:
#	% ./rundemo
# when you exit from one example, the next one will start and display on your
# screen.
#

rpdir=/opt/rappture

exdir=$rpdir/examples

dirs="app-fermi/tcl app-fermi/python app-fermi/fortran c-example graph zoo/structure zoo/image zoo/cloud"

echo "7 examples will be shown-->"
for i in $dirs
do
	echo $exdir/$i
	cd $exdir/$i
	$rpdir/bin/rappture
done

cd $exdir/3D
$rpdir/bin/rerun 3d_test_run.xml
