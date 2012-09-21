# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <boolean> elements
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

set choice [$driver get input.(iimodel).current]
$driver put output.boolean(outb).about.label "Echo of boolean value iimodel"
$driver put output.boolean(outb).current $choice

set choice1 [$driver get input.(iimodel1).current]
$driver put output.boolean(outb1).about.label "Echo of boolean value iimodel1"
$driver put output.boolean(outb1).current $choice1

set choice2 [$driver get input.(iimodel2).current]
$driver put output.boolean(outb2).about.label "Echo of boolean value iimodel2"
$driver put output.boolean(outb2).current $choice2

set choice3 [$driver get input.(iimodel3).current]
$driver put output.boolean(outb3).about.label "Echo of boolean value iimodel3"
$driver put output.boolean(outb3).current $choice3

# save the updated XML describing the run...
Rappture::result $driver
exit 0
