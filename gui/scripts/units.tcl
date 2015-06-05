# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: units - mechanism for converting numbers with units
#
#  These routines make it easy to define a system of units, to decode
#  numbers with units, and convert a number from one set of units to
#  another.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itcl

namespace eval Rappture { # forward declaration }
namespace eval Rappture::Units { # forward declaration }

# ----------------------------------------------------------------------
# USAGE: define units ?-type name? ?-metric boolean?
# USAGE: define units1->units2 {expr}
#
# Used to define a new fundamental type of units, or to define another
# system of units based on a fundamental type.  Once units are defined
# in this manner, the "convert" function can be used to convert a number
# in one system of units to another system.
# ----------------------------------------------------------------------
proc Rappture::Units::define {what args} {
    if {[regexp {(.+)->(.+)} $what match new fndm]} {
        if {[llength $args] != 2} {
            error "wrong # args: should be \"define units1->units2 exprTo exprFrom\""
        }
        #
        # Convert the units variables embedded in the conversion
        # expressions to something that Tcl can handle.  We'll
        # use ${number} to represent the variables.
        #
        foreach {exprTo exprFrom} $args { break }
        regsub -all $new $exprTo {${number}} exprTo
        regsub -all $fndm $exprFrom {${number}} exprFrom

        Rappture::Units::System #auto $new \
            -basis [list $fndm $exprTo $exprFrom]

    } elseif {[regexp {^/?[a-zA-Z]+[0-9]*$} $what]} {
        array set opts {
            -type ""
            -metric 0
        }
        foreach {key val} $args {
            if {![info exists opts($key)]} {
                error "bad option \"$key\": should be [join [lsort [array names opts]] {, }]"
            }
            set opts($key) $val
        }
        eval Rappture::Units::System #auto $what [array get opts]
    } else {
        error "bad units definition \"$what\": should be something like m or /cm3 or A->m"
    }
}

# ----------------------------------------------------------------------
# USAGE: convert value ?-context units? ?-to units? ?-units on/off?
#
# Used to convert one value with units to another value in a different
# system of units.  If the value has no units, then the units are taken
# from the -context, if that is supplied.  If the -to system is not
# specified, then the value is converted to fundamental units for the
# current system.
# ----------------------------------------------------------------------
proc Rappture::Units::convert {value args} {
    array set opts {
        -context ""
        -to ""
        -units "on"
    }
    foreach {key val} $args {
        if {![info exists opts($key)]} {
            error "bad option \"$key\": should be [join [lsort [array names opts]] {, }]"
        }
        set opts($key) $val
    }

    #
    # Parse the value into the number part and the units part.
    #
    set value [string trim $value]
    if {![regexp {^([-+]?[0-9]+\.?([0-9]+)?([eEdD][-+]?[0-9]+)?) *(/?[a-zA-Z]+[0-9]*)?$} $value match number dummy1 dummy2 units]} {
        set mesg "bad value \"$value\": should be real number with units"
        if {$opts(-context) != ""} {
            append mesg " of [Rappture::Units::description $opts(-context)]"
        }
        error $mesg
    }
    if {$units == ""} {
        set units $opts(-context)
    }

    #
    # Try to find the object representing the current system of units.
    #
    set units [Rappture::Units::System::regularize $units]
    set oldsys [Rappture::Units::System::for $units]
    if {$oldsys == ""} {
        set mesg "value \"$value\" has unrecognized units"
        if {$opts(-context) != ""} {
            append mesg ".\nShould be units of [Rappture::Units::description $opts(-context)]"
        }
        error $mesg
    }

    #
    # Convert the number to the new system of units.
    #
    if {$opts(-to) == ""} {
        # no units -- return the number as is
        return "$number$units"
    }
    return [$oldsys convert "$number$units" $opts(-to) $opts(-units)]
}

# ----------------------------------------------------------------------
# USAGE: description <units>
#
# Returns a description for the specified system of units.  The
# description includes the abstract type (length, temperature, etc.)
# along with a list of all compatible systems.
# ----------------------------------------------------------------------
proc Rappture::Units::description {units} {
    set sys [Rappture::Units::System::for $units]
    if {$sys == ""} {
        return ""
    }
    set mesg [$sys cget -type]
    set ulist [Rappture::Units::System::all $units]
    if {"" != $ulist} {
        append mesg " ([join $ulist {, }])"
    }
    return $mesg
}

# ----------------------------------------------------------------------
itcl::class Rappture::Units::System {
    public variable basis ""
    public variable type ""
    public variable metric 0

    constructor {name args} { # defined below }

    public method basic {}
    public method fundamental {}
    public method convert {value units showUnits}
    private variable _system ""  ;# this system of units

    public proc for {units}
    public proc all {units}
    public proc regularize {units}

    private common _base  ;# maps unit name => System obj

    # metric conversion prefixes
    private common _prefix2factor
    array set _prefix2factor {
        c  1e-2
        m  1e-3
        u  1e-6
        n  1e-9
        p  1e-12
        f  1e-15
        a  1e-18
        k  1e+3
        M  1e+6
        G  1e+9
        T  1e+12
        P  1e+15
    }
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Units::System::constructor {name args} {
    if {![regexp {^/?[a-zA-Z]+[0-9]*$} $name]} {
        error "bad units declaration \"$name\""
    }
    eval configure $args

    #
    # The -basis is a list {units exprTo exprFrom}, indicating the
    # fundamental system of units that this new system is based on,
    # and the expressions that can be used to convert this new system
    # to and from the fundamental system.
    #
    if {$basis != ""} {
        foreach {base exprTo exprFrom} $basis { break }
        if {![info exists _base($base)]} {
            error "fundamental system of units \"$base\" not defined"
        }
        while {$type == "" && $base != ""} {
            set obj $_base($base)
            set type [$obj cget -type]
            set base [lindex [$obj cget -basis] 0]
        }
    }
    set _system $name
    set _base($name) $this
}

# ----------------------------------------------------------------------
# USAGE: basic
#
# Returns the basic system of units for the current system.  The
# basic units may be the only units in this system.  But if this
# system has "-metric 1", the basic system is the system without
# any metric prefixes.
# ----------------------------------------------------------------------
itcl::body Rappture::Units::System::basic {} {
    return $_system
}

# ----------------------------------------------------------------------
# USAGE: fundamental
#
# Returns the fundamental system of units for the current system.
# For example, the current units might be degrees F, but the
# fundamental system might be degrees C.  The fundamental system
# depends on how each system is defined.  You can see it as the
# right-hand side of the -> arrow, as in "F->C".
# ----------------------------------------------------------------------
itcl::body Rappture::Units::System::fundamental {} {
    if {$basis != ""} {
        set sys [Rappture::Units::System::for [lindex $basis 0]]
        return [$sys fundamental]
    }
    return $_system
}

# ----------------------------------------------------------------------
# USAGE: convert value newUnits showUnits
#
# Converts a value with units to another value with the specified
# units.  The value must have units that are compatible with the
# current system.  Returns a string that represented the converted
# number and its new units.
# ----------------------------------------------------------------------
itcl::body Rappture::Units::System::convert {value newUnits showUnits} {
    if {![regexp {^([-+]?[0-9]+\.?([0-9]+)?([eEdD][-+]?[0-9]+)?) *(/?[a-zA-Z]+[0-9]*)?$} $value match number dummy1 dummy2 units]} {
        error "bad value \"$value\": should be real number with units"
    }

    #
    # Check the base units coming in.  They should match the base units
    # for the current system, or the base units for the fundamental basis.
    # If not, something went wrong with the caller.
    #
    set slash ""
    set prefix ""
    set power "1"
    if {$metric && [regexp {^(/?)([cmunpfakMGTP])([a-zA-Z]+)([0-9]*)$} $units match slash prefix base power]} {
        set baseUnits "$slash$base$power"
    } else {
        set baseUnits $units
    }
    if {![string equal $baseUnits $_system]
          && ![string equal $baseUnits [lindex $basis 0]]} {
        error "can't convert value \"$value\": should have units \"$_system\""
    }

    #
    # If the number coming in has a metric prefix, convert the number
    # to the base system.
    #
    if {$prefix != ""} {
        if {$power == ""} {
            set power 1
        }
        if {$slash == "/"} {
            set number [expr {$number/pow($_prefix2factor($prefix),$power)}]
        } else {
            set number [expr {$number*pow($_prefix2factor($prefix),$power)}]
        }
    }

    #
    # If the incoming units are a fundamental basis, then convert
    # the number from the basis to the current system.
    #
    if {[string equal $baseUnits [lindex $basis 0]]} {
        foreach {base exprTo exprFrom} $basis { break }
        set number [expr $exprFrom]
    }

    #
    # Check the base units for the new system of units.  If they match
    # the current system, then we're almost done.  Just handle the
    # metric prefix, if there is one.
    #
    set slash ""
    set prefix ""
    set power "1"
    if {$metric && [regexp {^(/?)([cmunpfakMGTP])([a-zA-Z]+)([0-9]*)$} $newUnits match slash prefix base power]} {
        set baseUnits "$slash$base$power"
    } else {
        set baseUnits $newUnits
    }
    if {[string equal $baseUnits $_system]} {
        if {$prefix != ""} {
            if {$power == ""} {
                set power 1
            }
            if {$slash == "/"} {
                set number [expr {$number*pow($_prefix2factor($prefix),$power)}]
            } else {
                set number [expr {$number/pow($_prefix2factor($prefix),$power)}]
            }
        }
        if {$showUnits} {
            return "$number$newUnits"
        }
        return $number
    }

    #
    # If we want a different system of units, then convert this number
    # to the fundamental basis.  If there is no fundamental basis, we
    # must already be in the fundamental basis.
    #
    set base $_system
    if {"" != $basis} {
        foreach {base exprTo exprFrom} $basis { break }
        set number [expr $exprTo]
    }

    set newsys [Rappture::Units::System::for $newUnits]
    return [$newsys convert "$number$base" $newUnits $showUnits]
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -basis
# ----------------------------------------------------------------------
itcl::configbody Rappture::Units::System::basis {
    if {[llength $basis] != 3} {
        error "bad basis \"$name\": should be {units exprTo exprFrom}"
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -metric
# ----------------------------------------------------------------------
itcl::configbody Rappture::Units::System::metric {
    if {![string is boolean -strict $metric]} {
        error "bad value \"$metric\": should be boolean"
    }
}

# ----------------------------------------------------------------------
# USAGE: for units
#
# Returns the System object for the given system of units, or ""
# if there is no system that matches the units string.
# ----------------------------------------------------------------------
itcl::body Rappture::Units::System::for {units} {
    #
    # See if the units are a recognized system.  If not, then try to
    # extract any metric prefix and see if what's left is a recognized
    # system.  If all else fails, see if we can find a system without
    # the exact capitalization.  The user might say "25c" instead of
    # "25C".  Try to allow that.
    #
    if {[info exists _base($units)]} {
        return $_base($units)
    } else {
        set orig $units
        if {[regexp {^(/?)[cCmMuUnNpPfFaAkKgGtT](.+)$} $units match slash tail]} {
            set base "$slash$tail"
            if {[info exists _base($base)]} {
                set sys $_base($base)
                if {[$sys cget -metric]} {
                    return $sys
                }
            }

            # check the base part for improper capitalization below...
            set units $base
        }

        set matching ""
        foreach u [array names _base] {
            if {[string equal -nocase $u $units]} {
                lappend matching $_base($u)
            }
        }
        if {[llength $matching] == 1} {
            set sys [lindex $matching 0]
            #
            # If we got rid of a metric prefix above, make sure
            # that the system is metric.  If not, then we don't
            # have a match.
            #
            if {[string equal $units $orig] || [$sys cget -metric]} {
                return $sys
            }
        }
    }
    return ""
}

# ----------------------------------------------------------------------
# USAGE: all units
#
# Returns a list of all units compatible with the given units string.
# Compatible units are determined by following all conversion
# relationships that lead to the same base system.
# ----------------------------------------------------------------------
itcl::body Rappture::Units::System::all {units} {
    set sys [Rappture::Units::System::for $units]
    if {$sys == ""} {
        return ""
    }

    if {"" != [$sys cget -basis]} {
        set basis [lindex [$sys cget -basis] 0]
    } else {
        set basis $units
    }

    set ulist $basis
    foreach u [array names _base] {
        set obj $_base($u)
        set b [lindex [$obj cget -basis] 0]
        if {$b == $basis} {
            lappend ulist $u
        }
    }
    return $ulist
}

# ----------------------------------------------------------------------
# USAGE: regularize units
#
# Examines the given expression of units and tries to regularize
# it so it has the proper capitalization.  For example, units like
# "/CM3" are converted to "/cm3".  If the units are not recognized,
# then they are returned as-is.
# ----------------------------------------------------------------------
itcl::body Rappture::Units::System::regularize {units} {
    set sys [for $units]
    if {$sys == ""} {
        return $units
    }
    # note: case-insensitive matching for metric prefix
    if {[regexp {^(/?)([cCmMuUnNpPfFaAkKgGtT]?)([a-zA-Z]+[0-9]+|[a-zA-Z]+)$} $units match slash prefix tail]} {
        if {[regexp {^[CUNFAK]$} $prefix]} {
            # we know that these should be lower case
            set prefix [string tolower $prefix]
        } elseif {[regexp {^[GT]$} $prefix]} {
            # we know that these should be upper case
            set prefix [string toupper $prefix]
        }
        return "$slash$prefix[string trimleft [$sys basic] /]"
    }
    return [$sys basic]
}

# ----------------------------------------------------------------------

# ----------------------------------------------------------------------
# Define common units...
# ----------------------------------------------------------------------
Rappture::Units::define m -type length -metric yes
Rappture::Units::define A->m {A*1.0e-10} {m*1.0e10}

Rappture::Units::define /m3 -type density -metric yes
Rappture::Units::define /m2 -type misc -metric yes

Rappture::Units::define C -type temperature -metric no
Rappture::Units::define K->C {K-273.15} {C+273.15}
Rappture::Units::define F->C {(F-32)/1.8} {(1.8*C)+32}

Rappture::Units::define eV -type energy -metric yes
Rappture::Units::define J->eV {J/1.602177e-19} {eV*1.602177e-19}

Rappture::Units::define V -type voltage -metric yes

Rappture::Units::define s -type seconds -metric yes
# can't use min becase tcl thinks its milli-in's
# Rappture::Units::define min->s {min*60.00} {s/60.00}
Rappture::Units::define h->s {h*3600.00} {s/3600.00}
Rappture::Units::define d->s {d*86400.00} {s/86400.00}

# can't put mol's in because tcl thinks its milli-ol's
# Rappture::Units::define mol -type misc -metric yes
Rappture::Units::define Hz -type misc -metric yes
Rappture::Units::define Bq -type misc -metric yes

Rappture::Units::define deg -type angle -metric no
Rappture::Units::define rad -type angle -metric no
Rappture::Units::define deg->rad {deg*(3.1415926535897931/180.00)} {rad*(180.00/3.1415926535897931)}
