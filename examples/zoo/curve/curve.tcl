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

# generate a single curve with x and y axis upper and lower limits
set xminl [expr $min+(($max-$min)/4.0)]
set xmaxl [expr $max-(($max-$min)/4.0)]
set yminl [expr {cos($xminl)/(1+$xminl)}]
set ymaxl [expr {cos($xmaxl)/(1+$xmaxl)}]

$driver put output.curve(limited).about.label "Axis limits curve"
$driver put output.curve(limited).about.description \
    "This is an example of a single curve with x and y axis limits applied."
$driver put output.curve(limited).xaxis.label "Time"
$driver put output.curve(limited).xaxis.description "Time during the experiment."
$driver put output.curve(limited).xaxis.units "s"
$driver put output.curve(limited).xaxis.min $xminl
$driver put output.curve(limited).xaxis.max $xmaxl
$driver put output.curve(limited).yaxis.label "Voltage v(11)"
$driver put output.curve(limited).yaxis.description "Output from the amplifier."
$driver put output.curve(limited).yaxis.units "V"
$driver put output.curve(limited).yaxis.min $yminl
$driver put output.curve(limited).yaxis.max $ymaxl

for {set x $min} {$x < $max} {set x [expr {$x+$dx}]} {
    set y [expr {cos($x)/(1+$x)}]
    $driver put -append yes output.curve(limited).component.xy "$x $y\n"
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
    #$driver put output.curve(multi$factor).xaxis.scale "log"
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

# generate a bar curve
$driver put output.curve(bars).about.label "Bar chart"
$driver put output.curve(bars).about.description \
    "This is an example of a scatter curve."
$driver put output.curve(bars).about.type "bar"
$driver put output.curve(bars).xaxis.label "Time"
$driver put output.curve(bars).xaxis.description "Time during the experiment."
$driver put output.curve(bars).xaxis.units "s"
$driver put output.curve(bars).yaxis.label "Voltage v(11)"
$driver put output.curve(bars).yaxis.description "Output from the amplifier."
$driver put output.curve(bars).yaxis.units "V"

for {set x 0} {$x < $npts} {incr x} {
    set y [expr {sin($x)/(1+$x)}]
    $driver put -append yes output.curve(bars).component.xy "$x $y\n"
}

# generate mixed curves on the same plot
set deg2rad 0.017453292519943295

$driver put output.curve(line).about.group "Mixed element types"
$driver put output.curve(line).about.label "Sine"
$driver put output.curve(line).about.description \
    "This is an example of a mixed curves on the same plot."
$driver put output.curve(line).xaxis.label "Degrees"
#$driver put output.curve(line).yaxis.label "Sine"
$driver put output.curve(line).about.type "line"
for {set x 0} {$x <= 360} {incr x 30} {
    set y [expr {sin($x*$deg2rad)}]
    $driver put -append yes output.curve(line).component.xy "$x $y\n"
}

$driver put output.curve(bar).about.group "Mixed element types"
$driver put output.curve(bar).about.label "Cosine"
$driver put output.curve(bar).about.description \
    "This is an example of a mixed curves on the same plot."
$driver put output.curve(bar).xaxis.label "Degrees"
#$driver put output.curve(bar).yaxis.label "Cosine"
$driver put output.curve(bar).about.type "bar"
$driver put output.curve(bar).about.style "-barwidth 24.0"

for {set x 0} {$x <= 360} {incr x 30} {
    set y [expr {cos($x*$deg2rad)}]
    $driver put -append yes output.curve(bar).component.xy "$x $y\n"
}

$driver put output.curve(point).about.group "Mixed element types"
$driver put output.curve(point).about.label "Random"
$driver put output.curve(point).about.description \
    "This is an example of a mixed curves on the same plot."
$driver put output.curve(point).xaxis.label "Degrees"
#$driver put output.curve(point).yaxis.label "Random"
$driver put output.curve(point).about.type "scatter"
for {set x 0} {$x <= 360} {incr x 10} {
    set y [expr {(rand() * 2.0) - 1}]
    $driver put -append yes output.curve(point).component.xy "$x $y\n"
}

# save the updated XML describing the run...
Rappture::result $driver
exit 0
