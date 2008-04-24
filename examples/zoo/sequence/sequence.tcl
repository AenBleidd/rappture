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
package require BLT
package require Img

# open the XML file containing the run parameters
set driver [Rappture::library [lindex $argv 0]]

set data [$driver get input.image.current]
set nframes [$driver get input.(nframes).current]

set imh [image create photo -data $data]
set wd [image width $imh]
set ht [image height $imh]

set zwd [expr {round(0.5*$wd)}]
set zht [expr {round(0.5*$ht)}]
set zoomi [image create photo -width $zwd -height $zht]

$driver put output.sequence(outs).about.label "Animated sequence"
$driver put output.sequence(outs).index.label "Frame"

for {set n 0} {$n < $nframes} {incr n} {
    set frac [expr {($n+1)/double($nframes)}]
    set x0 [expr {round(0.5*$frac*$wd)}]
    set y0 [expr {round(0.5*$frac*$ht)}]

    $zoomi copy $imh -from $x0 $y0

    set index [expr {$n+1}]
    $driver put output.sequence(outs).element($n).index $index
    $driver put output.sequence(outs).element($n).image.current \
        [$zoomi data -format jpeg]
    $driver put output.sequence(outs).element($n).about.label "$index of $nframes"
}

# save the updated XML describing the run...
Rappture::result $driver
exit 0
