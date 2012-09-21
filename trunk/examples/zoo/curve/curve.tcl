# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <curve> elements
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

set npts [$driver get input.(points).current]
set min 0.01
set max 10.0
set dx [expr {($max-$min)/double($npts)}]

# generate a single curve
$driver put output.curve(single).about.label "Single curve"
$driver put output.curve(single).about.description \
    "This is an example of a single curve."
$driver put output.curve(single).xaxis.label "Time"
$driver put output.curve(single).xaxis.description "Time during the experiment."
$driver put output.curve(single).xaxis.units "s"
$driver put output.curve(single).yaxis.label "Voltage v(11)"
$driver put output.curve(single).yaxis.description "Output from the amplifier."
$driver put output.curve(single).yaxis.units "V"

for {set x $min} {$x < $max} {set x [expr {$x+$dx}]} {
    set y [expr {cos($x)/(1+$x)}]
    $driver put -append yes output.curve(single).component.xy "$x $y\n"
}

# generate multiple curves on the same plot
foreach factor {1 2} {
    $driver put output.curve(multi$factor).about.group "Multiple curve"
    $driver put output.curve(multi$factor).about.label "Factor a=$factor"
    $driver put output.curve(multi$factor).about.description \
        "This is an example of a multiple curves on the same plot."
    $driver put output.curve(multi$factor).xaxis.label "Frequency"
    $driver put output.curve(multi$factor).xaxis.description \
        "Frequency of the input source."
    $driver put output.curve(multi$factor).xaxis.units "Hz"
    $driver put output.curve(multi$factor).xaxis.scale "log"
    $driver put output.curve(multi$factor).yaxis.label "Current"
    $driver put output.curve(multi$factor).yaxis.description \
        "Current through the pull-down resistor."
    $driver put output.curve(multi$factor).yaxis.units "uA"
    $driver put output.curve(multi$factor).yaxis.log "log"

    for {set x $min} {$x < $max} {set x [expr {$x+$dx}]} {
        set y [expr {pow(2.0,$factor*$x)/$x}]
        $driver put -append yes \
            output.curve(multi$factor).component.xy "$x $y\n"
    }
}

# generate a scatter curve
$driver put output.curve(scatter).about.label "Scatter curve"
$driver put output.curve(scatter).about.description \
    "This is an example of a scatter curve."
$driver put output.curve(scatter).about.type "scatter"
$driver put output.curve(scatter).xaxis.label "Time"
$driver put output.curve(scatter).xaxis.description "Time during the experiment."
$driver put output.curve(scatter).xaxis.units "s"
$driver put output.curve(scatter).yaxis.label "Voltage v(11)"
$driver put output.curve(scatter).yaxis.description "Output from the amplifier."
$driver put output.curve(scatter).yaxis.units "V"

for {set x $min} {$x < $max} {set x [expr {$x+$dx}]} {
    set y [expr {cos($x)/(1+$x)}]
    $driver put -append yes output.curve(scatter).component.xy "$x $y\n"
}

# save the updated XML describing the run...
Rappture::result $driver
exit 0
