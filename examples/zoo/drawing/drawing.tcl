# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <string> elements
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Rappture

# open the XML file containing the run parameters
set driver [Rappture::library [lindex $argv 0]]

set value [$driver get input.choice(analysis).current]
append out "Analysis Choice:  $value\n"
set value [$driver get input.number(trapezoid_top).current]
append out "Trapezoid Top:  $value\n"
set value [$driver get input.number(substrate_length).current]
append out "Substrate Length:  $value\n"
set value [$driver get input.number(feature_length).current]
append out "Feature Length:  $value\n"
set value [$driver get input.number(feature_height).current]
append out "Feature Height:  $value\n"
set value [$driver get input.string(indeck).current]
append out "String:  $value\n"

$driver put output.string(out).about.label "Echo of inputs"
$driver put output.string(out).current $out

# save the updated XML describing the run...
Rappture::result $driver
exit 0
