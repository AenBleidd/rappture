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

set s1 [$driver get input.phase(one).(first).current]
set s2 [$driver get input.phase(two).(second).current]

$driver put output.log "first = $s1\nsecond= $s2"

# save the updated XML describing the run...
Rappture::result $driver
exit 0
