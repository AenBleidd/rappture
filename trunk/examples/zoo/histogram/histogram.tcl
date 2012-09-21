# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <histogram> elements
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
set min 1
set max 10.0
set dx [expr {($max-$min)/double($npts)}]

# generate a single histogram
$driver put output.histogram(single).about.label "Single histogram"
$driver put output.histogram(single).about.description \
    "This is an example of a single histogram."
$driver put output.histogram(single).xaxis.label "Time"
$driver put output.histogram(single).xaxis.description "Time during the experiment."
$driver put output.histogram(single).xaxis.units "s"
$driver put output.histogram(single).yaxis.label "Voltage v(11)"
$driver put output.histogram(single).yaxis.description "Output from the amplifier."
$driver put output.histogram(single).yaxis.units "V"

for {set x $min} {$x < $max} {set x [expr {$x+$dx}]} {
    set y [expr {cos($x)/(1+$x)}]
    $driver put -append yes output.histogram(single).component.xy "$x $y\n"
}

# generate multiple histograms on the same plot
foreach factor {1 2} {
    $driver put output.histogram(multi$factor).about.group "Multiple histogram"
    $driver put output.histogram(multi$factor).about.label "Factor a=$factor"
    $driver put output.histogram(multi$factor).about.description \
        "This is an example of a multiple histograms on the same plot."
    $driver put output.histogram(multi$factor).xaxis.label "Frequency"
    $driver put output.histogram(multi$factor).xaxis.description \
        "Frequency of the input source."
    $driver put output.histogram(multi$factor).xaxis.units "Hz"
    $driver put output.histogram(multi$factor).yaxis.label "Current"
    $driver put output.histogram(multi$factor).yaxis.description \
        "Current through the pull-down resistor."
    $driver put output.histogram(multi$factor).yaxis.units "uA"
    $driver put output.histogram(multi$factor).yaxis.log "log"

    for {set x $min} {$x < $max} {set x [expr {$x+$dx}]} {
        set y [expr {pow(2.0,$factor*$x)/$x}]
        $driver put -append yes \
            output.histogram(multi$factor).component.xy "$x $y\n"
    }
}

# generate a name value histogram
set prefix output.histogram(namevalue)
$driver put $prefix.about.label "Name value histogram"
$driver put $prefix.about.description \
    "This is an example of a name value histogram."
$driver put $prefix.about.type "scatter"
$driver put $prefix.xaxis.label "Time"
$driver put $prefix.xaxis.description "Time during the experiment."
$driver put output.histogram(namevalue).xaxis.units "s"
$driver put output.histogram(namevalue).yaxis.label "Voltage v(11)"
$driver put output.histogram(namevalue).yaxis.description "Output from the amplifier."
$driver put output.histogram(namevalue).yaxis.units "V"

set labels { 
  Apple Bread Cumcumber Date Eel Fish Grape Hotdog "Ice Cream" 
  Java Knife L M N O P 
}
set count 0
for {set x $min} {$x < $max} {set x [expr {$x+$dx}]} {
    set y [expr {cos($x)/(1+$x)}]
    set name [lindex $labels $count]
    incr count
    $driver put -append yes $prefix.component.xy "[list $name $y]\n"
}

# save the updated XML describing the run...
Rappture::result $driver
exit 0
