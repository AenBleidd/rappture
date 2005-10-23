# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <field> elements
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
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

#
# Generate the 2D mesh and field values...
#
set n 0
set z 1
foreach {x y} {
    0 0
    1 0
    2 0
    3 0
    4 0
    0 1
    1 1
    2 1
    3 1
    4 1
    0 2
    1 2
    2 2
    3 2
    4 2
    0 3
    1 3
    2 3
    3 3
    4 3
    0 4
    1 4
    2 4
    3 4
    4 4
} {
    $driver put -append yes output.cloud(m2d).points "$x $y\n"
    incr n

    set fval [expr $formula]
    $driver put -append yes output.field(f2d).component.values "$fval\n"
}


#
# Generate the 3D mesh and field values...
#
set n 0
foreach {x y z} {
    0 0 0
    1 0 0
    2 0 0
    3 0 0
    4 0 0
    0 1 0
    1 1 0
    2 1 0
    3 1 0
    4 1 0
    0 2 0
    1 2 0
    2 2 0
    3 2 0
    4 2 0
    0 3 0
    1 3 0
    2 3 0
    3 3 0
    4 3 0
    0 4 0
    1 4 0
    2 4 0
    3 4 0
    4 4 0
    0 0 1
    1 0 1
    2 0 1
    3 0 1
    4 0 1
    0 1 1
    1 1 1
    2 1 1
    3 1 1
    4 1 1
    0 2 1
    1 2 1
    2 2 1
    3 2 1
    4 2 1
    0 3 1
    1 3 1
    2 3 1
    3 3 1
    4 3 1
    0 4 1
    1 4 1
    2 4 1
    3 4 1
    4 4 1
} {
    $driver put output.mesh(m3d).node($n) "$x $y $z"
    incr n

    set fval [expr $formula]
    $driver put -append yes output.field(f3d).component.values "$fval\n"
}

# meshes also need element descriptions
$driver put output.mesh(m3d).element(0).nodes "0 1 5 6 25 26 30 31"
$driver put output.mesh(m3d).element(1).nodes "1 2 6 7 26 27 31 32"
$driver put output.mesh(m3d).element(2).nodes "2 3 7 8 27 28 32 33"
$driver put output.mesh(m3d).element(3).nodes "3 4 8 9 28 29 33 34"
$driver put output.mesh(m3d).element(4).nodes "5 6 10 11 30 31 35 36"
$driver put output.mesh(m3d).element(5).nodes "8 9 13 14 33 34 38 39"
$driver put output.mesh(m3d).element(6).nodes "10 11 15 16 35 36 40 41"
$driver put output.mesh(m3d).element(7).nodes "13 14 18 19 38 39 43 44"
$driver put output.mesh(m3d).element(8).nodes "15 16 20 21 40 41 45 46"
$driver put output.mesh(m3d).element(9).nodes "16 17 21 22 41 42 46 47"
$driver put output.mesh(m3d).element(10).nodes "17 18 22 23 42 43 47 48"
$driver put output.mesh(m3d).element(12).nodes "18 19 23 24 43 44 48 49"

#
# Save the updated XML describing the run...
#
Rappture::result $driver
exit 0
