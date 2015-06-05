# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: NumberEntry - widget for entering numeric values
#
#  This widget represents a <number> entry on a control panel.
#  It is used to enter numeric values.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

itcl::class Rappture::NumberEntry {
    inherit itk::Widget

    itk_option define -state state State "normal"

    constructor {owner path args} { # defined below }

    public method value {args}

    public method label {}
    public method tooltip {}

    protected method _newValue {}

    private variable _owner ""    ;# thing managing this control
    private variable _path ""     ;# path in XML to this number
}

itk::usual NumberEntry {
    keep -cursor -font
    keep -foreground -background
    keep -textbackground
    keep -selectbackground -selectforeground -selectborderwidth
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::NumberEntry::constructor {owner path args} {
    if {[catch {$owner isa Rappture::ControlOwner} valid] != 0 || !$valid} {
        error "bad object \"$owner\": should be Rappture::ControlOwner"
    }
    set _owner $owner
    set _path $path

    #
    # Figure out what sort of control to create
    #
    set presets ""
    foreach pre [$_owner xml children -type preset $path] {
        set value [string trim [$_owner xml get $path.$pre.value]]
        set label [string trim [$_owner xml get $path.$pre.label]]
        lappend presets $value $label
    }

    set class Rappture::Gauge
    set units [string trim [$_owner xml get $path.units]]
    if {$units != ""} {
        set desc [Rappture::Units::description $units]
        if {[string match temperature* $desc]} {
            set class Rappture::TemperatureGauge
        }
    }

    #
    # Create the widget and configure it properly based on other
    # hints in the XML.
    #
    itk_component add gauge {
        $class $itk_interior.gauge -units $units -presets $presets -log $path
    }
    pack $itk_component(gauge) -expand yes -fill both
    bind $itk_component(gauge) <<Value>> [itcl::code $this _newValue]

    set min [string trim [$_owner xml get $path.min]]
    if {$min ne ""} {
        $itk_component(gauge) configure -minvalue $min
    }

    set max [string trim [$_owner xml get $path.max]]
    if {$max ne ""} {
        $itk_component(gauge) configure -maxvalue $max
    }

    if {$class == "Rappture::Gauge" && $min ne "" && $max ne ""} {
        set color [string trim [$_owner xml get $path.about.color]]
        if {$color == ""} {
            # deprecated.  Color should be in "about"
            set color [string trim [$_owner xml get $path.color]]
        }
        if {$color != ""}  {
            if {$units != ""} {
                set min [Rappture::Units::convert $min -to $units -units off]
                set max [Rappture::Units::convert $max -to $units -units off]
            }
            # For compatibility. If only one color use white for min
            if {[llength $color] == 1} {
                set color [list $min white $max $color]
            }
            $itk_component(gauge) configure \
                -spectrum [Rappture::Spectrum ::\#auto $color -units $units]
        }
    }

    # if the control has an icon, plug it in
    set str [string trim [$_owner xml get $path.about.icon]]
    if {$str ne ""} {
        $itk_component(gauge) configure -image [image create photo -data $str]
    }
    eval itk_initialize $args

    #
    # Assign the default value to this widget, if there is one.
    #
    set str [string trim [$_owner xml get $path.default]]
    if {$str ne ""} {
        $itk_component(gauge) value $str
    }
}

# ----------------------------------------------------------------------
# USAGE: value ?-check? ?<newval>?
#
# Clients use this to query/set the value for this widget.  With
# no args, it returns the current value for the widget.  If the
# <newval> is specified, it sets the value of the widget and
# sends a <<Value>> event.  If the -check flag is included, the
# new value is not actually applied, but just checked for correctness.
# ----------------------------------------------------------------------
itcl::body Rappture::NumberEntry::value {args} {
    set onlycheck 0
    set i [lsearch -exact $args -check]
    if {$i >= 0} {
        set onlycheck 1
        set args [lreplace $args $i $i]
    }

    if {[llength $args] == 1} {
        if {$onlycheck} {
            # someday we may add validation...
            return
        }
        set newval [string trim [lindex $args 0]]
        $itk_component(gauge) value $newval
        return $newval

    } elseif {[llength $args] != 0} {
        error "wrong # args: should be \"value ?-check? ?newval?\""
    }

    #
    # Query the value and return.
    #
    return [$itk_component(gauge) value]
}

# ----------------------------------------------------------------------
# USAGE: label
#
# Clients use this to query the label associated with this widget.
# Reaches into the XML and pulls out the appropriate label string.
# ----------------------------------------------------------------------
itcl::body Rappture::NumberEntry::label {} {
    set label [string trim [$_owner xml get $_path.about.label]]
    if {"" == $label} {
        set label "Number"
    }
    return $label
}

# ----------------------------------------------------------------------
# USAGE: tooltip
#
# Clients use this to query the tooltip associated with this widget.
# Reaches into the XML and pulls out the appropriate description
# string.  Returns the string that should be used with the
# Rappture::Tooltip facility.
# ----------------------------------------------------------------------
itcl::body Rappture::NumberEntry::tooltip {} {
    set str   [string trim [$_owner xml get $_path.about.description]]

    set units [string trim [$_owner xml get $_path.units]]
    set min   [string trim [$_owner xml get $_path.min]]
    set max   [string trim [$_owner xml get $_path.max]]

    if {$units != "" || $min != "" || $max != ""} {
        append str "\n\nEnter a number"

        if {$min != "" && $max != ""} {
            append str " between $min and $max"
        } elseif {$min != ""} {
            append str " greater than $min"
        } elseif {$max != ""} {
            append str " less than $max"
        }

        if {$units != ""} {
            set desc [Rappture::Units::description $units]
            append str " with units of $desc"
        }
    }
    return [string trim $str]
}

# ----------------------------------------------------------------------
# USAGE: _newValue
#
# Invoked automatically whenever the value in the gauge changes.
# Sends a <<Value>> event to notify clients of the change.
# ----------------------------------------------------------------------
itcl::body Rappture::NumberEntry::_newValue {} {
    event generate $itk_component(hull) <<Value>>
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -state
# ----------------------------------------------------------------------
itcl::configbody Rappture::NumberEntry::state {
    set valid {normal disabled}
    if {[lsearch -exact $valid $itk_option(-state)] < 0} {
        error "bad value \"$itk_option(-state)\": should be [join $valid {, }]"
    }
    $itk_component(gauge) configure -state $itk_option(-state)
}
