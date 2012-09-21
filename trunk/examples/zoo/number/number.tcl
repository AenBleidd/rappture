# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <number> elements
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

$driver put output.number(outt).about.label "Echo of temperature"
$driver copy output.number(outt) from input.(temperature)

$driver put output.number(outv).about.label "Echo of voltage sweep"
$driver copy output.number(outv) from input.(vsweep)

# save the updated XML describing the run...
Rappture::result $driver
exit 0
