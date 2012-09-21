# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <group> elements
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
