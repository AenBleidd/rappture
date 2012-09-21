# ----------------------------------------------------------------------
#  EXAMPLE: Optimization of the Rosenbrock function
#
#  This tool is a good example for optimization.  It implements the
#  Rosenbrock function:
#
#    f(x1,x2) = 100(x2 - x1**2)**2 + (1-x1)**2
#
#  This has a minimum at (x1,x2) = (1,1).
#
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

set x1 [$driver get input.(x1).current]
set x2 [$driver get input.(x2).current]

set f [expr {100*($x2-$x1*$x1)*($x2-$x1*$x1) + (1-$x1)*(1-$x1)}]

$driver put output.number(f).about.label "Rosenbrock function"
$driver put output.number(f).about.description \
    "f(x1,x2) = 100(x2 - x1**2)**2 + (1-x1)**2"
$driver put output.number(f).current $f

# save the updated XML describing the run...
Rappture::result $driver
exit 0
