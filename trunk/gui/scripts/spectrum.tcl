# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: spectrum - maps a range of real values onto a color
#
#  This data object represents the mapping of a range of real values
#  onto a range of colors.  It is used in conjunction with other
#  widgets, such as the Rappture::Gauge.
#
#  EXAMPLE:
#    Rappture::Spectrum #auto {
#       0.0   red
#       1.0   green
#      10.0   #d9d9d9
#    }
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

itcl::class Rappture::Spectrum {
    inherit Rappture::Dispatcher

    public variable units ""  ;# default units for all real values

    constructor {{sdata ""} args} { # defined below }

    public method insert {args}
    public method delete {first {last ""}}
    public method get {args}

    private variable _axis ""       ;# list of real values along axis
    private variable _rvals ""      ;# list of red components along axis
    private variable _gvals ""      ;# list of green components along axis
    private variable _bvals ""      ;# list of blue components along axis
    private variable _spectrum 0    ;# use continuous visible spectrum
    private variable _specv0 0      ;# minimum value
    private variable _specw0 0      ;# wavelength for minimum
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Spectrum::constructor {{sdata ""} args} {
    register !change  ;# used to signal changes in spectrum

    if {[llength $sdata] > 0} {
        regsub -all {\n} $sdata { } sdata
        eval insert $sdata
    }
    eval configure $args
}

# ----------------------------------------------------------------------
# USAGE: insert ?<value1> <color1> <value2> <color2> ...?
#
# Clients use this to insert one or more values into the spectrum.
# Each value has an associated color.  These values are used in the
# "get" method to map any incoming value to its interpolated color
# in the spectrum.
# ----------------------------------------------------------------------
itcl::body Rappture::Spectrum::insert {args} {
    set changed 0

    # special case. Two values in nm, then use
    # spectrum instead of gradient.
    if {[llength $args] == 4} {
        set cnt 0
        foreach {value color} $args {
            if {[string match "*nm" $color]} {
                incr cnt
            }
        }
        if {$cnt == 2} {
            set val0 [lindex $args 0]
            set color0 [string trimright [lindex $args 1] "nm"]
            set val1 [lindex $args 2]
            set color1 [string trimright [lindex $args 3] "nm"]

            if {"" != $units} {
                set val0 [Rappture::Units::convert $val0 \
                              -context $units -to $units -units off]
                set val1 [Rappture::Units::convert $val1 \
                              -context $units -to $units -units off]
            }

            set _spectrum [expr (double($color1) - double($color0)) \
                           / (double($val1) - double($val0))]
            set _specv0 $val0
            set _specw0 $color0
            return
        }
    }

    foreach {value color} $args {
        if {"" != $units} {
            set value [Rappture::Units::convert $value \
                -context $units -to $units -units off]
        }
        foreach {r g b} [Rappture::color::RGB $color] { break }
        set i 0
        foreach v $_axis {
            if {$value == $v} {
                set _rvals [lreplace $_rvals $i $i $r]
                set _gvals [lreplace $_gvals $i $i $g]
                set _bvals [lreplace $_bvals $i $i $b]
                set changed 1
                break
            } elseif {$value < $v} {
                set _axis  [linsert $_axis $i $value]
                set _rvals [linsert $_rvals $i $r]
                set _gvals [linsert $_gvals $i $g]
                set _bvals [linsert $_bvals $i $b]
                set changed 1
                break
            }
            incr i
        }

        if {$i >= [llength $_axis]} {
            lappend _axis $value
            lappend _rvals $r
            lappend _gvals $g
            lappend _bvals $b
            set changed 1
        }
    }

    # let any clients know if something has changed
    if {$changed} {
        event !change
    }
}

# ----------------------------------------------------------------------
# USAGE: delete <first> ?<last>?
#
# Clients use this to delete one or more entries from the spectrum.
# The <first> and <last> represent the integer index of the desired
# element.  If only <first> is specified, then that one element is
# deleted.  If <last> is specified, then all elements in the range
# <first> to <last> are deleted.
# ----------------------------------------------------------------------
itcl::body Rappture::Spectrum::delete {first {last ""}} {
    if {$last == ""} {
        set last $first
    }
    if {![regexp {^[0-9]+|end$} $first]} {
        error "bad index \"$first\": should be integer or \"end\""
    }
    if {![regexp {^[0-9]+|end$} $last]} {
        error "bad index \"$last\": should be integer or \"end\""
    }

    if {[llength [lrange $_axis $first $last]] > 0} {
        set _axis [lreplace $_axis $first $last]
        set _rvals [lreplace $_rvals $first $last]
        set _gvals [lreplace $_gvals $first $last]
        set _bvals [lreplace $_bvals $first $last]
        event !change
    }
}

# ----------------------------------------------------------------------
# USAGE: get ?-color|-fraction? ?<value>?
#
# Clients use this to get information about the spectrum.  With no args,
# it returns a list of elements in the form accepted by the "insert"
# method.  Otherwise, it returns the interpolated value for the given
# <value>.  By default, it returns the interpolated color, but the
# -fraction flag can be specified to query the fractional position
# along the spectrum.
# ----------------------------------------------------------------------
itcl::body Rappture::Spectrum::get {args} {
    if {[llength $args] == 0} {
        set rlist ""
        foreach v $_axis r $_rvals g $_gvals b $_bvals {
            lappend rlist "$v$units" [format {#%.4x%.4x%.4x} $r $g $b]
        }
        return $rlist
    }

    set what "-color"
    while {[llength $args] > 0} {
        set first [lindex $args 0]
        if {[regexp {^-[a-zA-Z]} $first]} {
            set what $first
            set args [lrange $args 1 end]
        } else {
            break
        }
    }
    if {[llength $args] != 1} {
        error "wrong # args: should be \"get ?-color|-fraction? ?value?\""
    }

    set value [lindex $args 0]

    switch -- [lindex $value 0] {
        gaussian {
            set value [lindex $value 1]
            if {$units != ""} {
                set value [Rappture::Units::convert $value \
                -context $units -to $units -units off]
            }
        }
        uniform {
            set min [lindex $value 1]
            set max [lindex $value 2]
            if {$units != ""} {
                set min [Rappture::Units::convert $min \
                -context $units -to $units -units off]
                set max [Rappture::Units::convert $max \
                -context $units -to $units -units off]
            }
            set value [expr {0.5 * ($min + $max)}]
        }
        default {
            if {$units != ""} {
                set value [Rappture::Units::convert $value \
                -context $units -to $units -units off]
            }
        }
    }

    switch -- $what {
        -color {
            if {$_spectrum != 0} {
                # continuous spectrum. just compute wavelength
                set waveln [expr ($value - $_specv0) * $_spectrum + $_specw0]
                return [Rappture::color::wave2RGB $waveln]
            }
            set i 0
            set ilast ""
            while {$i <= [llength $_axis]} {
                set v [lindex $_axis $i]

                if {$v == ""} {
                    set r [lindex $_rvals $ilast]
                    set g [lindex $_gvals $ilast]
                    set b [lindex $_bvals $ilast]
                    return [format {#%.4x%.4x%.4x} $r $g $b]
                } elseif {$value < $v} {
                    if {$ilast == ""} {
                        set r [lindex $_rvals $i]
                        set g [lindex $_gvals $i]
                        set b [lindex $_bvals $i]
                    } else {
                        set vlast [lindex $_axis $ilast]
                        set frac [expr {($value-$vlast)/double($v-$vlast)}]

                        set rlast [lindex $_rvals $ilast]
                        set r [lindex $_rvals $i]
                        set r [expr {round($frac*($r-$rlast) + $rlast)}]

                        set glast [lindex $_gvals $ilast]
                        set g [lindex $_gvals $i]
                        set g [expr {round($frac*($g-$glast) + $glast)}]

                        set blast [lindex $_bvals $ilast]
                        set b [lindex $_bvals $i]
                        set b [expr {round($frac*($b-$blast) + $blast)}]
                    }
                    return [format {#%.4x%.4x%.4x} $r $g $b]
                }
                set ilast $i
                incr i
            }
        }
        -fraction {
            set v0 [lindex $_axis 0]
            set vend [lindex $_axis end]
            return [expr {($value-$v0)/double($vend-$v0)}]
        }
        default {
            error "bad flag \"$what\": should be -color or -fraction"
        }
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTIONS: -units
# ----------------------------------------------------------------------
itcl::configbody Rappture::Spectrum::units {
    if {"" != $units && [Rappture::Units::System::for $units] == ""} {
        error "bad value \"$units\": should be system of units"
    }
    event !change
}
