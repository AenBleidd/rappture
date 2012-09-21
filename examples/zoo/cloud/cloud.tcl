# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <cloud> elements
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

# get the formula and make it ready for evaluation
set formula [$driver get input.string(formula).current]
regsub -all {[xyz]} $formula {$\0} formula

# get the number of points for the cloud
set npts [$driver get input.integer(npts).current]

#
# Generate the 2D mesh and field values...
#
$driver put output.cloud(m2d).about.label "2D Mesh"
$driver put output.cloud(m2d).units "um"
$driver put output.cloud(m2d).hide "yes"

$driver put output.field(f2d).about.label "2D Field"
$driver put output.field(f2d).component.mesh "output.cloud(m2d)"
set z 1
for {set n 0} {$n < $npts} {incr n} {
    set x [expr {rand()}]
    set y [expr {rand()}]
    $driver put -append yes output.cloud(m2d).points "$x $y\n"

    set fval [expr $formula]
    $driver put -append yes output.field(f2d).component.values "$fval\n"
}

#
# Generate the 3D mesh and field values...
#
$driver put output.cloud(m3d).about.label "3D Mesh"
$driver put output.cloud(m3d).units "um"
$driver put output.cloud(m3d).hide "yes"

$driver put output.field(f3d).about.label "3D Field"
$driver put output.field(f3d).component.mesh "output.cloud(m3d)"
for {set n 0} {$n < $npts} {incr n} {
    set x [expr {rand()}]
    set y [expr {rand()}]
    set z [expr {rand()}]
    $driver put -append yes output.cloud(m3d).points "$x $y $z\n"

    set fval [expr $formula]
    $driver put -append yes output.field(f3d).component.values "$fval\n"
}

#
# Save the updated XML describing the run...
#
Rappture::result $driver
exit 0
