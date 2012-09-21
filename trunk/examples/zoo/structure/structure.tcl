# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <structure> elements
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

set name [$driver get input.loader.current]

$driver put output.log "Structure: $name"

$driver copy output.structure from input.structure.current
$driver put output.structure.about.label "Structure"

# save the updated XML describing the run...
Rappture::result $driver
exit 0
