# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <log> elements
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005
#  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Rappture

# open the XML file containing the run parameters
set driver [Rappture::library [lindex $argv 0]]

set re [$driver get input.group.(models).(recomb).current]
set tn [$driver get input.group.(models).(tau).(taun).current]
set tp [$driver get input.group.(models).(tau).(taup).current]

set temp [$driver get input.group.(ambient).(temp).current]

$driver put output.log "Models:
  Recombination: $re
  taun = $tn
  taup = $tp

Ambient:
  tempK = $temp"

# save the updated XML describing the run...
Rappture::result $driver
exit 0
