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

$driver put output.cloud(m2d).about.label "2D Mesh"
$driver put output.cloud(m2d).units "um"
$driver put output.cloud(m2d).hide "yes"

$driver put output.field(f2d).about.label "2D Field"
$driver put output.field(f2d).component.mesh "output.cloud(m2d)"
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
# See what sort of 3D data to generate...
#
set vizmethod [$driver get input.choice(3D).current]

#
# Generate the 3D mesh and field values...
#
$driver put output.field(f3d).about.label "3D Field"
$driver put output.field(f3d).component.style "color blue:yellow:red"
if {$vizmethod == "vtk"} {
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
    $driver put output.mesh(m3d).about.label "3D Mesh"
    $driver put output.mesh(m3d).units "um"
    $driver put output.mesh(m3d).hide "yes"

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

    $driver put output.field(f3d).component.mesh "output.mesh(m3d)"
}

#
# Generate the 3D mesh in OpenDX format...
#
if {$vizmethod == "nanovis"} {
    set dx "object 1 class gridpositions counts 5 5 3
delta 1 0 0
delta 0 1 0
delta 0 0 1
object 2 class gridconnections counts 5 5 3
object 3 class array type double rank 0 items 75 data follows
"
    for {set z 0} {$z < 3} {incr z} {
        for {set y 0} {$y < 5} {incr y} {
            for {set x 0} {$x < 5} {incr x} {
                set fval [expr $formula]
                append dx "$fval\n"
            }
        }
    }
    append dx {attribute "dep" string "positions"
    object "regular positions regular connections" class field
    component "positions" value 1
    component "connections" value 2
    component "data" value 3}

    set file "/tmp/dx[clock seconds]"
    set fid [open $file w]
    puts $fid $dx
    close $fid

    set fid [open "| gzip -c $file | base64" r]
    $driver put output.field(f3d).component.dx [read $fid]
    close $fid

    file delete -force $file
}

#
# Save the updated XML describing the run...
#
Rappture::result $driver
exit 0
