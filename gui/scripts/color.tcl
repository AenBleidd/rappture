# ----------------------------------------------------------------------
#  UTILITY: color
#
#  This file contains various utility functions for manipulating
#  color values.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005
#  Purdue Research Foundation, West Lafayette, IN
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
