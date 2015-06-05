# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  UTILITY: color
#
#  This file contains various utility functions for manipulating
#  color values.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================

namespace eval Rappture::color { # forward declaration }

# ---------------------------------------------------------------------
# USAGE: brightness <color> <fraction>
#
# Used to brighten or dim a Tk <color> value by some +/- <fraction> in
# the range 0-1.  Returns another color in the same hue, but with
# different brighness.
# ----------------------------------------------------------------------
proc Rappture::color::brightness {color frac} {
    foreach {h s v} [Rappture::color::RGBtoHSV $color] { break }
    set v [expr {$v+$frac}]

    # if frac overflows the value, pass changes along to saturation
    if {$v < 0} {
        set s [expr {$s+$v}]
        if {$s < 0} { set s 0 }
        set v 0
    }
    if {$v > 1} {
        set s [expr {$s-($v-1)}]
        if {$s < 0} { set s 0 }
        set v 1
    }

    return [Rappture::color::HSVtoRGB $h $s $v]
}

# ---------------------------------------------------------------------
# USAGE: brightness_min <color> <min>
#
# Used to make sure a color is not too dim.  If the value is less
# than the <min>, then it is capped at that value.  That way, the
# return color shows up against black.
# ----------------------------------------------------------------------
proc Rappture::color::brightness_min {color min} {
    foreach {h s v} [Rappture::color::RGBtoHSV $color] { break }
    if {$v < $min} {
        set v $min
    }
    return [Rappture::color::HSVtoRGB $h $s $v]
}

# ---------------------------------------------------------------------
# USAGE: brightness_max <color> <max>
#
# Used to make sure a color is not too dim.  If the value is less
# than the <min>, then it is capped at that value.  That way, the
# return color shows up against black.
# ----------------------------------------------------------------------
proc Rappture::color::brightness_max {color max} {
    foreach {h s v} [Rappture::color::RGBtoHSV $color] { break }
    if {$v > $max} {
        set v $max
    }
    return [Rappture::color::HSVtoRGB $h $s $v]
}

# ---------------------------------------------------------------------
# USAGE: RGBtoHSV <color>
#
# Used to convert a Tk <color> value to hue, saturation, value
# components.  Returns a list of the form {h s v}.
# ----------------------------------------------------------------------
proc Rappture::color::RGBtoHSV {color} {
    #
    # If the colors are exhausted sometimes winfo can fail with a
    # division by zero.  Catch it to avoid problems.
    #
    if { [catch {winfo rgb . $color} status] != 0 } {
        set s 0
        set v 0
        set h 0
        return [list $h $s $v]
    }
    foreach {r g b} $status {}
    set min [expr {($r < $g) ? $r : $g}]
    set min [expr {($b < $min) ? $b : $min}]
    set max [expr {($r > $g) ? $r : $g}]
    set max [expr {($b > $max) ? $b : $max}]

    set v [expr {$max/65535.0}]

    set delta [expr {$max-$min}]

    if { $delta == 0 } {
        # delta=0 => gray color
        set s 0
        set v [expr {$r/65535.0}]
        set h 0
        return [list $h $s $v]
    }
 
    if {$max > 0} {
        set s [expr {$delta/double($max)}]
    } else {
        # r=g=b=0  =>  s=0, v undefined
        set s 0
        set v 0
        set h 0
        return [list $h $s $v]
    }

    if {$r == $max} {
        set h [expr {($g - $b)/double($delta)}]
    } elseif {$g == $max} {
        set h [expr {2 + ($b - $r)/double($delta)}]
    } else {
        set h [expr {4 + ($r - $g)/double($delta)}]
    }
    set h [expr {$h*1.04719756667}] ;# *60 degrees
    if {$h < 0} {
        set h [expr {$h+6.2831854}]
    }
    return [list $h $s $v]
}

# ---------------------------------------------------------------------
# USAGE: HSVtoRGB <color>
#
# Used to convert hue, saturation, value for a color to its
# equivalent RGB form.  Returns a prompt Tk color of the form
# #RRGGBB.
# ----------------------------------------------------------------------
proc Rappture::color::HSVtoRGB {h s v} {
    if {$s == 0} {
        set v [expr round(255*$v)]
        set r $v
        set g $v
        set b $v
    } else {
        if {$h >= 6.28318} {set h [expr $h-6.28318]}
        set h [expr $h/1.0472]
        set f [expr $h-floor($h)]
        set p [expr round(255*$v*(1.0-$s))]
        set q [expr round(255*$v*(1.0-$s*$f))]
        set t [expr round(255*$v*(1.0-$s*(1.0-$f)))]
        set v [expr round(255*$v)]

        switch [expr int($h)] {
            0 {set r $v; set g $t; set b $p}
            1 {set r $q; set g $v; set b $p}
            2 {set r $p; set g $v; set b $t}
            3 {set r $p; set g $q; set b $v}
            4 {set r $t; set g $p; set b $v}
            5 {set r $v; set g $p; set b $q}
        }
    }
    return [format "#%.2x%.2x%.2x" $r $g $b]
}

# ---------------------------------------------------------------------
# USAGE: wave2RGB <wavelength>
#
# Given a visible wavelength in nm, returns a Tk color of the form
# #RRGGBB. Returns black for nonvisible wavelengths.  Based on code from
# Dan Bruton (astro@tamu.edu) http://www.physics.sfasu.edu/astro/color/spectra.html
# ----------------------------------------------------------------------

proc Rappture::color::wave2RGB {wl} {

    # strip off any units
    set wl [string trimright $wl "nm"]

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

# Returns a list containing three decimal values in the range 0 to 65535,
# which are the red, green, and blue intensities that correspond to color
# in the window given by window. Color may be specified in any of the forms
# acceptable for a color option.
proc Rappture::color::RGB {color} {
    if {[string match "*nm" $color]} {
        set color [Rappture::color::wave2RGB [string trimright $color "nm"]]
    }
    return [winfo rgb . $color]
}
