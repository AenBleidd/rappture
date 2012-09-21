# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <string> elements
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

set title [$driver get input.(title).current]
set indeck [$driver get input.(indeck).current]

$driver put output.string(outt).about.label "Echo of title"
$driver put output.string(outt).current $title
$driver put output.string(outi).about.label "Echo of input"
$driver put output.string(outi).current $indeck

# save the updated XML describing the run...
Rappture::result $driver
exit 0
