# ----------------------------------------------------------------------
#  EXAMPLE: Fermi-Dirac function in Tcl.
#
#  This simple example shows how to use Rappture within a simulator
#  written in Tcl.
# ======================================================================
#  AUTHOR:  Derrick Kearney, Purdue University
#  Copyright (c) 2005-2009  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Rappture

# open the XML file containing the run parameters
set lib [Rappture::Library [lindex $argv 0]]

if {[$lib error] != 0} {
    # cannot open file or out of memory
    puts stderr [$lib traceback]
    exit [$lib error]
}

set T [Rappture::connect $lib "temperature"]
set Ef [$lib value "Ef" "units=eV"]

if {[$lib error != 0]} {
    # there were errors while retrieving input data values
    # dump the tracepack
    puts stderr [$lib traceback]
    exit [$lib error]
}

set nPts 200

set kT [expr {8.61734e-5 * [$T value "K"]}]
set Emin [expr {$Ef - 10*$kT}]
set Emax [expr {$Ef + 10*$kT}]

set dE [expr {0.005*($Emax-$Emin)}]

set E $Emin
for {set idx 0} {idx < nPts} {incr idx} {
    set E [expr {$E + $dE}]
    set f [expr {1.0/(1.0 + exp(($E - $Ef)/$kT))}]
    lappend fArr $f
    lappend Err $E
    set progress [expr {(($E - $Emin)/($Emax - $Emin)*100)}]
    Rappture::Utils::progress $progress -mesg "Iterating"
}

set curveLabel "Fermi-Dirac Curve"
set curveDesc "Plot of Fermi-Dirac Calculation"

# do it the easy way,
# create a plot to add to the library
# plot is registered with lib upon object creation
# p1->add(nPts,xArr,yArr,format,curveLabel,curveDesc);

set p1 [Rappture::Plot $lib]
$p1 add $nPts $fArr $EArr "" $curveLabel $curveDesc
$p1 propstr "xlabel" "Fermi-Dirac Factor"
$p1 propstr "ylabel" "Energy"
$p1 propstr "yunits" "eV"

$lib result

exit 0
