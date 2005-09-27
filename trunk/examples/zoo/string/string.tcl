# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <string> elements
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005
#  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Rappture

# open the XML file containing the run parameters
set driver [Rappture::library [lindex $argv 0]]

set title [$driver get input.(title).current]
set indeck [$driver get input.(indeck).current]

$driver put output.string(outt).current $title
$driver put output.string(outi).current $indeck

# save the updated XML describing the run...
Rappture::result $driver
exit 0
