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
set avals [$driver get input.string(avals).current]
set bvals [$driver get input.string(bvals).current]

# remove any commas in the avals/bvals lists
regsub -all {,} $avals {} avals
regsub -all {,} $bvals {} bvals

# change "x" to $x in expression
regsub -all {x} $func {$x} func
regsub -all {A} $func {$A} func
regsub -all {B} $func {$B} func

set xmin -1
set xmax 1
set npts 30
set elem output.sequence(outs).about.label "Sequence of Plots"
set elem output.sequence(outs).index.label "Parameter A"
foreach A $avals {
    set elem output.sequence(outs).element($A)
    $driver put $elem.index $A

    if {[llength $bvals] > 0} {
        # one or more B values -- put out a separate curve for each B value
        foreach B $bvals {
            $driver put $elem.curve($B).about.label "B = $B"
            $driver put $elem.curve($B).about.group "A = $A"
            $driver put $elem.curve($B).xaxis.label "x"
            $driver put $elem.curve($B).yaxis.label "Function y(x)"

            for {set i 0} {$i < $npts} {incr i} {
                set x [expr {$i*($xmax-$xmin)/double($npts) + $xmin}]
                set y [expr $func]
                $driver put -append yes \
                  $elem.curve($B).component.xy "$x $y\n"
            }
        }
    } else {
        # no B values -- put out a single curve for each element
        $driver put $elem.curve.xaxis.label "x"
        $driver put $elem.curve.yaxis.label "Function y(x)"

        for {set i 0} {$i < $npts} {incr i} {
            set x [expr {$i*($xmax-$xmin)/double($npts) + $xmin}]
            set y [expr $func]
            $driver put -append yes \
              output.sequence(outs).element($A).curve.component.xy "$x $y\n"
        }
    }
}

# save the updated XML describing the run...
Rappture::result $driver
exit 0
