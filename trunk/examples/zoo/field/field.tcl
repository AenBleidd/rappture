# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <field> elements
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

#
# Generate a uniform grid 2D mesh and field values...
#

$driver put output.mesh(m2d).about.label "2D Mesh"
$driver put output.mesh(m2d).dim 2
$driver put output.mesh(m2d).units "um"
$driver put output.mesh(m2d).hide "yes"
$driver put output.mesh(m2d).grid.xaxis.min 0.0
$driver put output.mesh(m2d).grid.xaxis.max 4.0
$driver put output.mesh(m2d).grid.xaxis.numpoints 5
$driver put output.mesh(m2d).grid.yaxis.min 0.0
$driver put output.mesh(m2d).grid.yaxis.max 4.0
$driver put output.mesh(m2d).grid.yaxis.numpoints 5

$driver put output.field(f2d).about.label "2D Field"
$driver put output.field(f2d).component.mesh "output.mesh(m2d)"

set z 1
for {set y 0} {$y < 5} {incr y} {
    for {set x 0} {$x < 5} {incr x} {
        set fval [expr $formula]
        $driver put -append yes output.field(f2d).component.values "$fval\n"
    }
}

#
# See what sort of 3D data to generate...
#
set vizmethod [$driver get input.choice(3D).current]

if {$vizmethod == "grid"} {
    #
    # Generate a uniform grid 3D mesh and field values...
    #
    $driver put output.mesh(m3d).about.label "3D Uniform Mesh"
    $driver put output.mesh(m3d).dim 3
    $driver put output.mesh(m3d).units "um"
    $driver put output.mesh(m3d).hide "yes"
    $driver put output.mesh(m3d).grid.xaxis.min 0.0
    $driver put output.mesh(m3d).grid.xaxis.max 4.0
    $driver put output.mesh(m3d).grid.xaxis.numpoints 5
    $driver put output.mesh(m3d).grid.yaxis.min 0.0
    $driver put output.mesh(m3d).grid.yaxis.max 4.0
    $driver put output.mesh(m3d).grid.yaxis.numpoints 5
    $driver put output.mesh(m3d).grid.zaxis.min 0.0
    $driver put output.mesh(m3d).grid.zaxis.max 1.0
    $driver put output.mesh(m3d).grid.zaxis.numpoints 2

    $driver put output.field(f3d).about.label "3D Field"
    $driver put output.field(f3d).component.mesh "output.mesh(m3d)"

    for {set z 0} {$z < 2} {incr z} {
        for {set y 0} {$y < 5} {incr y} {
            for {set x 0} {$x < 5} {incr x} {
                set fval [expr $formula]
                $driver put -append yes output.field(f3d).component.values "$fval\n"
            }
        }
    }
}

if {$vizmethod == "unstructured"} {
    #
    # Generate an unstructured grid 3D mesh and field values...
    #
    $driver put output.mesh(m3d).about.label "3D Unstructured Mesh"
    $driver put output.mesh(m3d).dim 3
    $driver put output.mesh(m3d).units "um"
    $driver put output.mesh(m3d).hide "yes"

    $driver put output.field(f3d).about.label "3D Field"
    $driver put output.field(f3d).component.mesh "output.mesh(m3d)"

    for {set z 0} {$z < 2} {incr z} {
        for {set y 0} {$y < 5} {incr y} {
            for {set x 0} {$x < 5} {incr x} {
                $driver put -append yes output.mesh(m3d).unstructured.points "$x $y $z\n"
                set fval [expr $formula]
                $driver put -append yes output.field(f3d).component.values "$fval\n"
            }
        }
    }

    $driver put -append yes output.mesh(m3d).unstructured.hexahedrons "0 1 6 5 25 26 31 30\n"
    $driver put -append yes output.mesh(m3d).unstructured.hexahedrons "1 2 7 6 26 27 32 31\n"
    $driver put -append yes output.mesh(m3d).unstructured.hexahedrons "2 3 8 7 27 28 33 32\n"
    $driver put -append yes output.mesh(m3d).unstructured.hexahedrons "3 4 9 8 28 29 34 33\n"
    $driver put -append yes output.mesh(m3d).unstructured.hexahedrons "5 6 11 10 30 31 36 35\n"
    $driver put -append yes output.mesh(m3d).unstructured.hexahedrons "8 9 14 13 33 34 39 38\n"
    $driver put -append yes output.mesh(m3d).unstructured.hexahedrons "10 11 16 15 35 36 41 40\n"
    $driver put -append yes output.mesh(m3d).unstructured.hexahedrons "13 14 19 18 38 39 44 43\n"
    $driver put -append yes output.mesh(m3d).unstructured.hexahedrons "15 16 21 20 40 41 46 45\n"
    $driver put -append yes output.mesh(m3d).unstructured.hexahedrons "16 17 22 21 41 42 47 46\n"
    $driver put -append yes output.mesh(m3d).unstructured.hexahedrons "17 18 23 22 42 43 48 47\n"
    $driver put -append yes output.mesh(m3d).unstructured.hexahedrons "18 19 24 23 43 44 49 48\n"
}

#
# Save the updated XML describing the run...
#
Rappture::result $driver
exit 0
