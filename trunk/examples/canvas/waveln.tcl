# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <number> elements
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Rappture
package require Img
wm withdraw .

set spectrum {
    380 #ff00ff
    390 #e300ff
    400 #c700ff
    410 #ab00ff
    420 #9000ff
    430 #7400ff
    440 #5800ff
    450 #3c00ff
    460 #2100ff
    470 #0500ff
    480 #0023db
    490 #004faf
    500 #007b83
    510 #00a757
    520 #00d32b
    530 #00ff00
    540 #33ff00
    550 #66ff00
    560 #99ff00
    570 #ccff00
    580 #ffff00
    590 #ffe500
    600 #ffcc00
    610 #ffb200
    620 #ff9900
    630 #ff7f00
    640 #ff6600
    650 #ff4c00
    660 #ff3200
    670 #ff1900
    680 #ff0000
    690 #e50000
    700 #cc0000
    710 #b20000
    720 #990000
    730 #7f0000
}

# open the XML file containing the run parameters
set xml [Rappture::library [lindex $argv 0]]
set wavel [$xml get input.number(wavel).current]
set wavel [Rappture::Units::convert $wavel -to nm -units off]

set tooldir [file dirname [info script]]
set psfile "plot[pid].ps"
set jpgfile "plot[pid].jpg"

canvas .c

#
# Create the diagram on the canvas.
#
set x0 20
set x1 280
set xmid [expr {0.5*($x0+$x1)}]
set nx [expr {[llength $spectrum]/2}]
set dx [expr {($x1-$x0)/double($nx)}]

set y0 35
set y1 65

set x $x0
foreach {wl color} $spectrum {
    .c create rectangle $x $y0 [expr {$x+$dx}] $y1 -outline "" -fill $color
    set x [expr {$x+$dx}]
}
.c create rectangle $x0 $y0 $x1 $y1 -outline black -fill ""
.c create text $xmid [expr {$y1+6}] \
    -anchor n -text "Visible Spectrum" -font "Helvetica 18"

set x [expr {($x1-$x0)/double(740-380) * ($wavel-380) + $x0}]

set imh [image create photo -file [file join $tooldir hand.gif]]
.c create image $x $y0 -image $imh
.c create text $x [expr {$y0-15}] -anchor s -text "$wavel nm"

#
# Convert the PostScript from the canvas into an image.
#
set psdata [.c postscript -width 300 -height 100]
set fid [open $psfile w]
puts $fid $psdata
close $fid

set status [catch {exec convert $psfile $jpgfile} result]

if {$status == 0} {
    set dest [image create photo -format jpeg -file $jpgfile]
    $xml put output.image(diagram).current [$dest data -format jpeg]
    $xml put output.image(diagram).about.label "Visible Spectrum"
    file delete -force $psfile $jpgfile
} else {
    puts stderr "ERROR during postscript conversion:\n$result"
    file delete -force $psfile $jpgfile
    exit 1
}

# save the updated XML describing the run...
Rappture::result $xml
exit 0
