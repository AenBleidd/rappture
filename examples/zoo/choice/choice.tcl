# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <choice> elements
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

$driver put output.choice(outs).about.label "Echo of choice"
set choice [$driver get input.(stats).current]
$driver put output.choice(outs).current $choice

# save the updated XML describing the run...
Rappture::result $driver
exit 0
