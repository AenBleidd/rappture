# ----------------------------------------------------------------------
#  COMPONENT: NumberEntry - widget for entering numeric values
#
#  This widget represents a <number> entry on a control panel.
#  It is used to enter numeric values.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005
#  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Itk

itcl::class Rappture::NumberEntry {
    inherit itk::Widget

    constructor {xmlobj path args} { # defined below }

    public method value {args}

    public method label {}
    public method tooltip {}

    protected method _newValue {}

    private variable _xmlobj ""   ;# XML containing description
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
itcl::body Rappture::NumberEntry::constructor {xmlobj path args} {
    if {![Rappture::library isvalid $xmlobj]} {
        error "bad value \"$xmlobj\": should be Rappture::library"
    }
    set _xmlobj $xmlobj
    set _path $path

    #
    # Figure out what sort of control to create
    #
    set presets ""
    foreach pre [$xmlobj children -type preset $path] {
        lappend presets \
            [$xmlobj get $path.$pre.value] \
            [$xmlobj get $path.$pre.label]
    }

    set class Rappture::Gauge
    set units [$xmlobj get $path.units]
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
        $class $itk_interior.gauge -units $units -presets $presets
    }
    pack $itk_component(gauge) -expand yes -fill both
    bind $itk_component(gauge) <<Value>> [itcl::code $this _newValue]

    set min [$xmlobj get $path.min]
    if {"" != $min} { $itk_component(gauge) configure -minvalue $min }

    set max [$xmlobj get $path.max]
    if {"" != $max} { $itk_component(gauge) configure -maxvalue $max }

    if {$class == "Rappture::Gauge" && "" != $min && "" != $max} {
        set color [$xmlobj get $path.color]
        if {$color == ""} {
            set color blue
        }
        if {$units != ""} {
            set min [Rappture::Units::convert $min -to $units -units off]
            set max [Rappture::Units::convert $max -to $units -units off]
        }
        $itk_component(gauge) configure \
            -spectrum [Rappture::Spectrum ::#auto [list \
                $min white $max $color] -units $units]
    }

    # if the control has an icon, plug it in
    set str [$xmlobj get $path.about.icon]
    if {$str != ""} {
        $itk_component(gauge) configure -image [image create photo -data $str]
    }

    eval itk_initialize $args

    #
    # Assign the default value to this widget, if there is one.
    #
    set str [$xmlobj get $path.default]
    if {"" != $str != ""} { $itk_component(gauge) value $str }
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
        set newval [lindex $args 0]
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
    set label [$_xmlobj get $_path.about.label]
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
    set str [$_xmlobj get $_path.about.description]

    set units [$_xmlobj get $_path.units]
    set min [$_xmlobj get $_path.min]
    set max [$_xmlobj get $_path.max]

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
