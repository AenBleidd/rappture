# ----------------------------------------------------------------------
#  Visualize the output of genetic optimization
#
#  This script reads a bunch of runXXXX.xml files and plots them
#  so you can see where the genetic algorithm was simulating and
#  what it came up with.
#
#  Run this as:  wish visualize.tcl
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require BLT
package require Rappture

# ----------------------------------------------------------------------
#  Load up all of the Rappture result files
# ----------------------------------------------------------------------
set jobs 0
foreach file [lsort [glob run*.xml]] {
    set result($jobs) [Rappture::library $file]
    incr jobs
}

blt::graph .space
.space xaxis configure -title "x1" -min -2 -max 2
.space yaxis configure -title "y1" -min -2 -max 2
.space legend configure -hide no
pack .space -side left -expand yes -fill both

blt::graph .value
.value xaxis configure -title "job number" -min 0 -max $jobs
.value yaxis configure -title "f(x1,x2)" -min 0 -max 2
.value legend configure -hide yes
pack .value -side left -expand yes -fill both

set colors {blue purple magenta green yellow orange tomato red black}
for {set pop [expr [llength $colors]-1]} {$pop >= 0} {incr pop -1} {
    blt::vector x1vec$pop
    blt::vector x2vec$pop
    .space element create spots$pop -xdata x1vec$pop -ydata x2vec$pop \
        -color [lindex $colors $pop] -linewidth 0 -label "Population #$pop"

    blt::vector jobvec$pop
    blt::vector fvec$pop
    .value element create line$pop -xdata jobvec$pop -ydata fvec$pop \
        -color [lindex $colors $pop] -symbol none
}

for {set n 0} {$n < $jobs} {incr n} {
    set pop [expr {$n/100}]
    x1vec$pop append [$result($n) get input.number(x1).current]
    x2vec$pop append [$result($n) get input.number(x2).current]
    jobvec$pop append $n
    fvec$pop append [$result($n) get output.number(f).current]
    update
    after 100
}
