# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <periodicelement> element
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

set first [$driver get input.periodicelement(first).current]
set second [$driver get input.periodicelement(second).current]
set third [$driver get input.periodicelement(third).current]

$driver put output.log "Elements:
  First:	$first
  Second:	$second
  Third:	$third"

# save the updated XML describing the run...
Rappture::result $driver
exit 0
