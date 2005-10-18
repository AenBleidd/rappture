# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <cloud> elements
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005
#  Purdue Research Foundation, West Lafayette, IN
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
