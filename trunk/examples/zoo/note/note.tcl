# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <note> elements
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2007  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Rappture

# open the XML file containing the run parameters
set driver [Rappture::library [lindex $argv 0]]

$driver copy output.number(diameter) from input.(diameter)

# save the updated XML describing the run...
Rappture::result $driver
exit 0
