# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <loader> elements
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

set one [$driver get input.(one).current]
set two [$driver get input.(two).current]

$driver put output.log "Input #1: $one\nInput #2: $two"

# save the updated XML describing the run...
Rappture::result $driver
exit 0
