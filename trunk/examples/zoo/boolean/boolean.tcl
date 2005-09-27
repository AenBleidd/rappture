# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <boolean> elements
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005
#  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Rappture

# open the XML file containing the run parameters
set driver [Rappture::library [lindex $argv 0]]

set choice [$driver get input.(iimodel).current]
$driver put output.boolean(outb).current $choice

# save the updated XML describing the run...
Rappture::result $driver
exit 0
