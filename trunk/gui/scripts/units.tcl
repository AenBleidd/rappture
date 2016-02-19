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
# USAGE: mcheck_range value {min ""} {max ""}
#
# Checks a value or PDF to determine if is is in a required range.
# Automatically does unit conversion if necessary.
# Returns value if OK.  Error if out-of-range
# Examples:
#    [mcheck_range "gaussian 0C 1C" 200K 500K] returns 1
#    [mcheck_range "uniform 100 200" 150 250] returns 0
#    [mcheck_range 100 0 200] returns 1
# ----------------------------------------------------------------------

proc Rappture::Units::_check_range {value min max units} {
    # puts "_check_range $value min=$min max=$max units=$units"
    # make sure the value has units
    if {$units != ""} {
        set value [Rappture::Units::convert $value -context $units]
        # for comparisons, remove units
        set nv [Rappture::Units::convert $value -context $units -units off]
        # get the units for the value
        set newunits [Rappture::Units::Search::for $value]
    } else {
        set nv $value
    }

    if {"" != $min} {
        if {"" != $units} {
            # compute the minimum in the new units
            set minv [Rappture::Units::convert $min -to $newunits -context $units  -units off]
            # same, but include units for printing
            set convMinVal [Rappture::Units::convert $min -to $newunits -context $units]
        } else {
            set minv $min
            set convMinVal $min
        }
        if {$nv < $minv} {
            error "Minimum value allowed here is $convMinVal"
        }
    }
    if {"" != $max} {
        if {"" != $units} {
            # compute the maximum in the new units
            set maxv [Rappture::Units::convert $max -to $newunits -context $units -units off]
            # same, but include units for printing
            set convMaxVal [Rappture::Units::convert $max -to $newunits -context $units ]
        } else {
            set maxv $max
            set convMaxVal $max
        }
        if {$nv > $maxv} {
            error "Maximum value allowed here is $convMaxVal"
        }
    }
    return $value
}

proc Rappture::Units::mcheck_range {value {min ""} {max ""} {units ""}} {
    # puts "mcheck_range $value min=$min max=$max units=$units"

    switch -- [lindex $value 0] {
        normal -
        gaussian {
            # get the mean
            set mean [_check_range [lindex $value 1] $min $max $units]
            if {$units == ""} {
                set dev [lindex $value 2]
                set ndev $dev
            } else {
                set dev [Rappture::Units::convert [lindex $value 2] -context $units]
                set ndev [Rappture::Units::convert $dev -units off]
            }
            if {$ndev <= 0} {
                error "Deviation must be positive."
            }
            return [list gaussian $mean $dev]
        }
        uniform {
            set min [_check_range [lindex $value 1] $min $max $units]
            set max [_check_range [lindex $value 2] $min $max $units]
            return [list uniform $min $max]
        }
        exact  {
            return [_check_range [lindex $value 1] $min $max $units]
        }
        default {
            return [_check_range [lindex $value 0] $min $max $units]
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: mconvert value ?-context units? ?-to units? ?-units on/off?
#
# This version of convert() converts multiple values.  Used when the
# value could be a range or probability density function (PDF).
# Examples:
#    gaussian 100k 1k
#    uniform 0eV 10eV
#    42
#    exact 42
# ----------------------------------------------------------------------

proc Rappture::Units::mconvert {value args} {
    # puts "mconvert $value : $args"
    array set opts {
        -context ""
        -to ""
        -units "on"
    }

    set value [split $value]

    switch -- [lindex $value 0] {
        normal - gaussian {
            set valtype gaussian
            set vals [lrange $value 1 2]
            set convtype {0 1}
        }
        uniform {
            set valtype uniform
            set vals [lrange $value 1 2]
            set convtype {0 0}
        }
        exact  {
            set valtype ""
            set vals [lindex $value 1]
            set convtype {0}
        }
        default {
            set valtype ""
            set vals $value
            set convtype {0}
        }
    }

    foreach {key val} $args {
        if {![info exists opts($key)]} {
            error "bad option \"$key\": should be [join [lsort [array names opts]] {, }]"
        }
        set opts($key) $val
    }

    set newval $valtype
    foreach val $vals ctype $convtype {
        if {$ctype == 1} {
            # This code handles unit conversion for deltas (changes).
            # For example, if we want a standard deviation of 10C converted
            # to Kelvin, that is 10K, NOT a standard deviation of 283.15K.
            set units [Rappture::Units::Search::for $val]
            set base [eval Rappture::Units::convert 0$units $args -units off]
            set new [eval Rappture::Units::convert $val $args -units off]
            set delta [expr $new - $base]
            set val $delta$opts(-to)
        }
        # tcl 8.5 allows us to do this:
        # lappend newval [Rappture::Units::convert $val {*}$args]
        # but we are using tcl8.4 so we use eval :^(
        lappend newval [eval Rappture::Units::convert $val $args]
    }
    return $newval
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
# proc Rappture::Units::convert {value args} {}
# Actual implementation is in rappture/lang/tcl/src/RpUnitsTclInterface.cc.


# ----------------------------------------------------------------------
# USAGE: description <units>
#
# Returns a description for the specified system of units.  The
# description includes the abstract type (length, temperature, etc.)
# along with a list of all compatible systems.
# ----------------------------------------------------------------------
# proc Rappture::Units::description {units} {}
# Actual implementation is in rappture/lang/tcl/src/RpUnitsTclInterface.cc.


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

    # These are in rappture/lang/tcl/src/RpUnitsTclInterface.cc.
    # public proc for {units}
    # public proc all {units}

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
# itcl::body Rappture::Units::System::for {units} {}
# Actual implementation is in rappture/lang/tcl/src/RpUnitsTclInterface.cc.


# ----------------------------------------------------------------------
# USAGE: all units
#
# Returns a list of all units compatible with the given units string.
# Compatible units are determined by following all conversion
# relationships that lead to the same base system.
# ----------------------------------------------------------------------
# itcl::body Rappture::Units::System::all {units} {}
# Actual implementation is in rappture/lang/tcl/src/RpUnitsTclInterface.cc.


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
