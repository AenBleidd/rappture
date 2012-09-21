# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <number> elements
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Rappture
package require Tk
package require Img

# ---------------------------------------------------------------------
# USAGE: wave2RGB <wavelength>
#
# Given a visible wavelength in nm, returns a Tk color of the form
# #RRGGBB. Returns black for nonvisible wavelengths.  Based on code from
# Dan Bruton (astro@tamu.edu) http://www.physics.sfasu.edu/astro/color/spectra.html
# ----------------------------------------------------------------------

proc wave2RGB {wl} {
    if {$wl < 380 || $wl > 780} {
	return black
    }
    set gamma 0.8
    set r 0.0
    set g 0.0
    set b 0.0
    if {$wl <= 440} {
	set r [expr (440.0 - $wl) / 60.0]
	set b 1.0
    } elseif {$wl <= 490} {
	set g [expr ($wl - 440.0) / 50.0]
	set b 1.0
    } elseif {$wl <= 510} {
	set g 1.0
	set b [expr (510.0 - $wl) / 20.0]
    } elseif {$wl <= 580} {
	set g 1.0
	set r [expr ($wl - 510.0) / 70.0]
    } elseif {$wl <= 645} {
	set r 1.0
	set g [expr (645.0 - $wl) / 65.0]
    } else {
	set r 1.0
    }

    if {$wl > 700} {
	set sss [expr 0.3 + 0.7 * (780.0 - $wl) / 80.0]
    } elseif {$wl < 420} {
	set sss [expr 0.3 + 0.7 * ($wl - 380.0) / 40.0]
    } else {
	set sss 1.0
    }
    set r [expr int(255.0 * pow(($sss * $r), $gamma))]
    set g [expr int(255.0 * pow(($sss * $g), $gamma))]
    set b [expr int(255.0 * pow(($sss * $b), $gamma))]
    return [format "#%.2X%.2X%.2X" $r $g $b]
}


wm withdraw .

# open the XML file containing the run parameters
set xml [Rappture::library [lindex $argv 0]]
set wavel [$xml get input.number(wavel).current]
set wavel [Rappture::Units::convert $wavel -to nm -units off]

set tooldir [file dirname [info script]]
set psfile "plot[pid].ps"
set pngfile "plot[pid].png"


# height and width of spectrum image
# It will get scaled to fit window, but render at high
# resolution so it looks good when large.
set width 720
set height 50

# load the hand image
set imh [image create photo -file [file join $tooldir hand.gif]]

# Where to place the spectrum image. Leave room for the hand above it.
set x0 [image width $imh]
set y0 [image height $imh]

# create canvas big enough to hold images
canvas .c -height [expr $height + $y0 + 100] -width [expr $width + $x0 + 50]

# spectrum range
set start 380
set stop 740

# calculate some offsets
set x1 [expr $x0 + $width]
set y1 [expr $y0 + $height]
set xmid [expr {0.5*($x0+$x1)}]
set dwl [expr ($stop - $start)/$width.0]

# draw spectrum image
set x $x0
for {set wl $start} {$wl < $stop} {set wl [expr $wl + $dwl]} {
    set color [wave2RGB $wl]
    .c create rectangle $x $y0 [expr $x+1] $y1 -outline "" -fill $color
    incr x
}
.c create rectangle $x0 $y0 $x1 $y1 -outline black -fill ""
.c create text $xmid [expr {$y1+20}] -anchor n -text "Visible Spectrum" -font "Helvetica 18"

# now place the hand image
set x [expr {($x1-$x0)/double($stop-$start) * ($wavel-380) + $x0}]
.c create image $x $y0 -image $imh
.c create text $x [expr {$y0-25}] \
    -anchor s -text "$wavel nm" -font "Helvetica 14"

#
# Convert the PostScript from the canvas into an image.
#
set psdata [.c postscript -height [expr $height + $y0 + 100] -width [expr $width + $x0 + 50]]
set fid [open $psfile w]
puts $fid $psdata
close $fid

# convert postscript to png image
set status [catch {exec convert $psfile $pngfile} result]

if {$status == 0} {
    set dest [image create photo -format png -file $pngfile]
    $xml put output.image(diagram).current [$dest data -format png]
    $xml put output.image(diagram).about.label "Visible Spectrum"
    file delete -force $psfile $pngfile
} else {
    puts stderr "ERROR during postscript conversion:\n$result"
    file delete -force $psfile $pngfile
    exit 1
}

# save the updated XML describing the run...
Rappture::result $xml
exit 0
