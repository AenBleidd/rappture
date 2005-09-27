# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <number> elements
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005
#  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Rappture

# open the XML file containing the run parameters
set driver [Rappture::library [lindex $argv 0]]

set T [$driver get input.(temperature).current]
set v [$driver get input.(vsweep).current]

$driver put output.number(outt).current $T
$driver put output.number(outv).current $v

# save the updated XML describing the run...
Rappture::result $driver
exit 0
