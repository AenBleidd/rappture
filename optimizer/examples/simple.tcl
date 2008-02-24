# ----------------------------------------------------------------------
#  EXAMPLE:  Rappture optimization
#
#  This script shows the core of the Rappture optimization loop.
#  It represents a simple skeleton of the much more complicated
#  Rappture GUI, but it drives enough of the optimization process
#  to drive development and testing.
#
#  Run this as:  wish simple.tcl ?-tool path/to/tool.xml?
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2008  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require BLT
package require Itcl
package require Rappture
package require RapptureGUI
package require RapptureOptimizer

set popsize 100  ;# size of each population for genetic algorithm

# ----------------------------------------------------------------------
#  Create a Tool object based on the tool.xml file...
# ----------------------------------------------------------------------
Rappture::getopts argv params {
    value -tool tool.xml
}

# open the XML file containing the tool parameters
if {![file exists $params(-tool)]} {
    puts stderr "can't find tool \"$params(-tool)\""
    exit 1
}
set xmlobj [Rappture::library $params(-tool)]

set installdir [file dirname $params(-tool)]
if {"." == $installdir} {
    set installdir [pwd]
}

set tool [Rappture::Tool ::#auto $xmlobj $installdir]

# ----------------------------------------------------------------------
#  Create some plotting stuff so we can watch the progress of the
#  optimization as it runs.
# ----------------------------------------------------------------------
blt::graph .space
.space xaxis configure -title "x1" -min -2 -max 2
.space yaxis configure -title "x2" -min -2 -max 2
.space legend configure -hide no
pack .space -side left -expand yes -fill both

blt::graph .value
.value xaxis configure -title "job number" -min 0
.value yaxis configure -title "f(x1,x2)" -logscale yes -max 1
.value legend configure -hide yes
pack .value -side left -expand yes -fill both

set colors {magenta purple blue DeepSkyBlue cyan green yellow Gold orange tomato red FireBrick black}

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

set jobnumber 0
proc add_to_plot {xmlobj} {
    global jobnumber popsize
    set pop [expr {$jobnumber/$popsize}]
    x1vec$pop append [$xmlobj get input.number(x1).current]
    x2vec$pop append [$xmlobj get input.number(x2).current]
    jobvec$pop append $jobnumber
    fvec$pop append [$xmlobj get output.number(f).current]
    incr jobnumber
}

# ----------------------------------------------------------------------
#  Create an optimization context and configure the parameters used
#  for optimization...
# ----------------------------------------------------------------------
Rappture::optimizer optim -tool $tool -using pgapack

optim add number input.number(x1) -min -2 -max 2
optim add number input.number(x2) -min -2 -max 2
optim configure -operation minimize -popsize $popsize -maxruns 1000

set status [optim perform \
    -fitness output.number(f).current \
    -updatecommand add_to_plot]

puts "done: $status"
