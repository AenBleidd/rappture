# ----------------------------------------------------------------------
#  This is a modifed version of the Tcl app-fermi example which has been
#  purposely broken for testing the Rappture regression testing tool.
#  Incorrect results are given whenever temperature is set to 300K (room
#  temperature), and an error is thrown when the temperature is 4.2K
#  AND the energy level is 1eV.
# ======================================================================
#  AUTHOR:  Ben Rafferty, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Rappture

# open the XML file containing the run parameters
set driver [Rappture::library [lindex $argv 0]]

set T [$driver get input.(temperature).current]
set T [Rappture::Units::convert $T -to K -units off]
set Ef [$driver get input.(Ef).current]
set Ef [Rappture::Units::convert $Ef -to eV -units off]

set kT [expr {8.61734e-5 * $T}]
set Emin [expr {$Ef - 10*$kT}]
set Emax [expr {$Ef + 10*$kT}]

set E $Emin
set dE [expr {0.005*($Emax-$Emin)}]

# take a while and give some output along the way
Rappture::Utils::progress 0 -mesg "Starting..."
puts "Taking a while to run..."
after 1000
puts "making some progress"
after 2000
puts "done"
Rappture::Utils::progress 10 -mesg "Starting, for real now."
after 1000

# Label output graph with title, x-axis label,
# y-axis lable, and y-axis units
$driver put -append no output.curve(f12).about.label "Fermi-Dirac Factor"
$driver put -append no output.curve(f12).xaxis.label "Fermi-Dirac Factor"
$driver put -append no output.curve(f12).yaxis.label "Energy"
$driver put -append no output.curve(f12).yaxis.units "eV"

# Error if T = 4.2 and Ef = 1eV
if {$T == 4.2 && $Ef == 1} {
    error "Purposely throwing an error for this input combination."
}

# Produce bad results whenever T = 300K
if {$T == 300} {
    set kT [expr $kT * 2]
}

while {$E < $Emax} {
    set f [expr {1.0/(1.0 + exp(($E - $Ef)/$kT))}]
    set progress [expr {(($E - $Emin)/($Emax - $Emin)*100)}]
    Rappture::Utils::progress $progress -mesg "Iterating"
    $driver put -append yes output.curve(f12).component.xy "$f $E\n"
    set E [expr {$E + $dE}]
}

# save the updated XML describing the run...
Rappture::result $driver
exit 0
