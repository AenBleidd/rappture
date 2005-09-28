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

set one [$driver get input.(one).current]
set two [$driver get input.(two).current]

$driver put output.log "Input #1: $one\nInput #2: $two"

# save the updated XML describing the run...
Rappture::result $driver
exit 0
