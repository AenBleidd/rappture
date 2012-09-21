# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <energies> elements
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Rappture

# open the XML file containing the run parameters
set driver [Rappture::library [lindex $argv 0]]

set L [$driver get input.number(L).current]
set L [Rappture::Units::convert $L -to "m" -units off]

set emass [$driver get input.number(emass).current]
set m [expr {$emass*9.11e-31}]  ;# kg

set h 4.13566743e-15  ;# in eVs
set J2eV 6.241506363e17

set nhomo [expr {round(rand()*19+1)}]

$driver put output.table.about.label "Energy Levels"
$driver put output.table.column(labels).label "Name"
$driver put output.table.column(energies).label "Energy"
$driver put output.table.column(energies).units "eV"
for {set n 1} {$n <= 20} {incr n} {
    set E [expr {$n*$n*$h*$h/(8.0*$m*$L*$L*$J2eV)}]  ;# in eV
    set label [expr {($n == $nhomo) ? "HOMO" : $n}]

    $driver put -append yes output.table.data [format "%s %.3g\n" $label $E]
}

# save the updated XML describing the run...
Rappture::result $driver
exit 0
