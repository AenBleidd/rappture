# ----------------------------------------------------------------------
#  EXAMPLE:  Rappture optimization
#
#  This script shows the core of the Rappture optimization loop.
#  It represents a simple skeleton of the much more complicated
#  Rappture GUI, but it drives enough of the optimization process
#  to drive development and testing.
#
#  Run this as:  tclsh simple.tcl ?-tool path/to/tool.xml?
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2008  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itcl
package require Rappture
package require RapptureGUI
package require RapptureOptimizer

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
#  Create an optimization context and configure the parameters used
#  for optimization...
# ----------------------------------------------------------------------
Rappture::optimizer optim -tool $tool -using pgapack

optim add number input.number(x1) -min -2 -max 2
optim add number input.number(x2) -min -2 -max 2
optim configure -operation minimize -popsize 100 -maxruns 200

set status [optim perform \
    -fitness output.number(f).current \
    -updatecommand {puts "checking"}]

puts "done: $status"
