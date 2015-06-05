# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: BooleanEntry - widget for entering boolean values
#
#  This widget represents a <boolean> entry on a control panel.
#  It is used to enter yes/no or on/off values.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

itcl::class Rappture::BooleanEntry {
    inherit itk::Widget

    itk_option define -state state State "normal"

    constructor {owner path args} { # defined below }

    public method value {args}

    public method label {}
    public method tooltip {}

    protected method _newValue {}
    protected method _log {}

    private variable _owner ""    ;# thing managing this control
    private variable _path ""     ;# path in XML to this number
}

itk::usual BooleanEntry {
    keep -cursor -font
    keep -foreground -background
    keep -textbackground
    keep -selectbackground -selectforeground -selectborderwidth
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::BooleanEntry::constructor {owner path args} {
    if {[catch {$owner isa Rappture::ControlOwner} valid] != 0 || !$valid} {
        error "bad object \"$owner\": should be Rappture::ControlOwner"
    }
    set _owner $owner
    set _path $path

    itk_component add -protected vframe {
        frame $itk_interior.vframe
    }

    # if the control has an icon, plug it in
    set icon [string trim [$_owner xml get $path.about.icon]]
    if {$icon != ""} {
        itk_component add icon {
            set icon [image create photo -data $icon]
            set w [image width $icon]
            set h [image height $icon]
            canvas $itk_component(vframe).icon -height $h -width $w -borderwidth 0 -highlightthickness 0
        } {
            usual
            ignore -highlightthickness -highlightbackground -highlightcolor
        }
        set c $itk_component(icon)
        $c create image [expr {0.5*$w}] [expr {0.5*$h}] \
            -anchor center -image $icon
        pack $itk_component(icon) -fill x -side left
    }

    #
    # Create the widget and configure it properly based on other
    # hints in the XML.
    #
    itk_component add switch {
        Rappture::Switch $itk_component(vframe).switch \
            -interactcommand [itcl::code $this _log]
    }
    set color [string trim [$_owner xml get $path.about.color]]
    if {$color != ""} {
        $itk_component(switch) configure -oncolor $color
    }

    bind $itk_component(switch) <<Value>> [itcl::code $this _newValue]
    pack $itk_component(switch) -fill x -side left
    pack $itk_component(vframe) -side left -expand yes -fill both
    eval itk_initialize $args

    #
    # Assign the default value to this widget, if there is one.
    #
    set str [string trim [$_owner xml get $path.default]]
    if {"" != $str} {
        $itk_component(switch) value $str
    } else {
        $itk_component(switch) value off
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
itcl::body Rappture::BooleanEntry::value {args} {
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
        $itk_component(switch) value $newval
        event generate $itk_component(hull) <<Value>>
        return $newval

    } elseif {[llength $args] != 0} {
        error "wrong # args: should be \"value ?-check? ?newval?\""
    }

    #
    # Query the value and return.
    #
    return [$itk_component(switch) value]
}

# ----------------------------------------------------------------------
# USAGE: label
#
# Clients use this to query the label associated with this widget.
# Reaches into the XML and pulls out the appropriate label string.
# ----------------------------------------------------------------------
itcl::body Rappture::BooleanEntry::label {} {
    set label [string trim [$_owner xml get $_path.about.label]]
    if {"" == $label} {
        set label "Boolean"
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
itcl::body Rappture::BooleanEntry::tooltip {} {
    set str [string trim [$_owner xml get $_path.about.description]]
    append str "\n\nClick to turn on/off"
    return $str
}

# ----------------------------------------------------------------------
# USAGE: _newValue
#
# Invoked automatically whenever the value in the gauge changes.
# Sends a <<Value>> event to notify clients of the change.
# ----------------------------------------------------------------------
itcl::body Rappture::BooleanEntry::_newValue {} {
    event generate $itk_component(hull) <<Value>>
}

# ----------------------------------------------------------------------
# USAGE: _log
#
# Used internally to send info to the logging mechanism.  Calls the
# Rappture::Logger mechanism to log the change to this input.
# ----------------------------------------------------------------------
itcl::body Rappture::BooleanEntry::_log {} {
    Rappture::Logger::log input $_path [$itk_component(switch) value]
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -state
# ----------------------------------------------------------------------
itcl::configbody Rappture::BooleanEntry::state {
    set valid {normal disabled}
    if {[lsearch -exact $valid $itk_option(-state)] < 0} {
        error "bad value \"$itk_option(-state)\": should be [join $valid {, }]"
    }
    $itk_component(switch) configure -state $itk_option(-state)
}
