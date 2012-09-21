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
set result [Rappture::Interface::connect "factorsTable"]

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

# store results in the results table
# add data to the table pointed to by the variable result.
# put the fArr data in the column named "Fermi-Dirac Factor"
# put the EArr data in the column named "Energy"
#
# Rappture::Table Commands:
# the store command overwrites data that already exists in
# the column with the new array of data.
# the append command will append the new array of data onto
# any previously existing array of data.

$result store "Fermi-Dirac Factor" $fArr
$result store "Energy" $EArr

Rappture::Interface::close

exit 0
