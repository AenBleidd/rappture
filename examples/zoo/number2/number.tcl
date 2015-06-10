# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <number> elements
# ======================================================================
#  AUTHOR:  Martin Hunt, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Rappture

# open the XML file containing the run parameters
set driver [Rappture::library [lindex $argv 0]]

set n1 [$driver get input.(input1).current]
set n2 [$driver get input.(input2).current]
set n3 [$driver get input.(input3).current]

$driver put output.string(out).current "input1=$n1\n"
$driver put -append yes output.string(out).current "input2=$n2\n"
$driver put -append yes output.string(out).current "input3=$n3\n"

# save the updated XML describing the run...
Rappture::result $driver
exit 0
