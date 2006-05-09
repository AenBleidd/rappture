# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <sequence> elements
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Rappture
package require Tk
wm withdraw .

# open the XML file containing the run parameters
set driver [Rappture::library [lindex $argv 0]]

set func [$driver get input.string(func).current]
set pvals [$driver get input.string(pvals).current]

# remove any commas in the pvals list
regsub -all {,} $pvals {} pvals

# change "x" to $x in expression
regsub -all {x} $func {$x} func
regsub -all {P} $func {$P} func

set xmin -1
set xmax 1
set npts 30
foreach P $pvals {
    $driver put output.sequence(outs).element($P).index $P
    $driver put output.sequence(outs).element($P).curve.xaxis.label "x"
    $driver put output.sequence(outs).element($P).curve.yaxis.label \
        "Function y(x)"

    for {set i 0} {$i < $npts} {incr i} {
        set x [expr {$i*($xmax-$xmin)/double($npts) + $xmin}]
        set y [expr $func]
        $driver put -append yes \
          output.sequence(outs).element($P).curve.component.xy "$x $y\n"
    }
}

# save the updated XML describing the run...
Rappture::result $driver
exit 0
