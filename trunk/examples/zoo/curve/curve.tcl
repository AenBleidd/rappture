# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <boolean> elements
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005
#  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Rappture

# open the XML file containing the run parameters
set driver [Rappture::library [lindex $argv 0]]

set npts [$driver get input.(points).current]
set min 0.01
set max 10.0
set dx [expr {($max-$min)/double($npts)}]

# generate a single curve
for {set x $min} {$x < $max} {set x [expr {$x+$dx}]} {
    set y [expr {cos($x)/(1+$x)}]
    $driver put -append yes output.curve(single).component.xy "$x $y\n"
}

# generate multiple curves on the same plot
foreach factor {1 2} {
    for {set x $min} {$x < $max} {set x [expr {$x+$dx}]} {
        set y [expr {pow(2.0,$factor*$x)/$x}]
        $driver put -append yes \
            output.curve(multi$factor).component.xy "$x $y\n"
    }
}

# save the updated XML describing the run...
Rappture::result $driver
exit 0
