# ----------------------------------------------------------------------
#  EXAMPLE: Fermi-Dirac function in Tcl.
#
#  This simple example shows how to use Rappture within a simulator
#  written in Tcl.
# ======================================================================
#  AUTHOR:  Derrick Kearney, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Rappture

# initialize the global interface
Rappture::Interface $argv fermi_io

# check the global interface for errors
if {[Rappture::Interface::error] != 0} {
    # there were errors while setting up the inteface
    # dump the traceback
    set o [Rappture::Interface::outcome]
    puts stderr [$o context]
    puts stderr [$o remark]
    exit [Rappture::Interface::error]
}

# connect variables to the interface
# look in the global interface for an object named
# "temperature, convert its value to Kelvin, and
# store the value into the address of T.
# look in the global interface for an object named
# "Ef", convert its value to electron Volts and store
# the value into the address of Ef
# look in the global interface for an object named
# factorsTable and set the variable result to
# point to it.
set T [Rappture::Interface::connect "temperature" -hints {"units=K"}]
set Ef [Rappture::Interface::connect "Ef" -hints {"units=eV"}]
set p1 [Rappture::Interface::connect "fdfPlot"]
set p2 [Rappture::Interface::connect "fdfPlot2"]

if {[Rappture::Interface::error] != 0]} {
    # there were errors while retrieving input data values
    # dump the tracepack
    set o [Rappture::Interface::outcome]
    puts stderr [$o context]
    puts stderr [$o remark]
    exit [Rappture::Interface::error]
}

# do science calculations
set nPts 200

set kT [expr {8.61734e-5 * $T}]
set Emin [expr {$Ef - 10*$kT}]
set Emax [expr {$Ef + 10*$kT}]

set dE [expr {(1.0/$nPts)*($Emax-$Emin)}]

set E $Emin
for {set idx 0} {idx < nPts} {incr idx} {
    set E [expr {$E + $dE}]
    set f [expr {1.0/(1.0 + exp(($E - $Ef)/$kT))}]
    lappend fArr $f
    lappend Err $E
    set progress [expr {(($E - $Emin)/($Emax - $Emin)*100)}]
    Rappture::Utils::progress $progress -mesg "Iterating"
}

# set up the curves for the plot by using the add command
# add <name> <xdata> <ydata> -format <fmt>
#
# to group curves on the same plot, just keep adding curves
# to save space, X array values are compared between curves.
# the the X arrays contain the same values, we only store
# one version in the internal data table, otherwise a new
# column is created for the array. for big arrays this may take
# some time, we should benchmark to see if this can be done
# efficiently.

$p1 add "fdfCurve1" $fArr $EArr -format "g:o"

$p2 add "fdfCurve2" $fArr $EArr -format "b-o"
$p2 add "fdfCurve3" $fArr $EArr -format "p--"

# close the global interface
# signal to the graphical user interface that science
# calculations are complete and to display the data
# as described in the views
Rappture::Interface::close

exit 0
