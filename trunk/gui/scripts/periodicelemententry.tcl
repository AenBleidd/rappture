# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: PeriodicElementEntry - widget for entering a choice of strings
#
#  This widget represents a <element> entry on a control panel.
#  It is used to choose one of several mutually-exclusive strings.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

itcl::class Rappture::PeriodicElementEntry {
    inherit itk::Widget

    itk_option define -state state State "normal"

    constructor {owner path args} { # defined below }
    destructor { # defined below }

    public method value {args}

    public method label {}
    public method tooltip {}

    protected method _newValue {}
    protected method _tooltip {}

    private variable _owner ""    ;# thing managing this control
    private variable _path ""     ;# path in XML to this number
}

itk::usual PeriodicElementEntry {
    keep -cursor -font
    keep -foreground -background
    keep -textforeground -textbackground
    keep -selectbackground -selectforeground -selectborderwidth
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::PeriodicElementEntry::constructor {owner path args} {
    if {[catch {$owner isa Rappture::ControlOwner} valid] != 0 || !$valid} {
        error "bad object \"$owner\": should be Rappture::ControlOwner"
    }
    set _owner $owner
    set _path $path

    set defval [string trim [$_owner xml get $_path.default]]
    # Active and inactive are lists.  Don't need to trim.
    set active [string trim [$_owner xml get $_path.active]]
    set inactive [string trim [$_owner xml get $_path.inactive]]
    #
    # Create the widget and configure it properly based on other
    # hints in the XML.
    #
    itk_component add element {
        Rappture::PeriodicElement $itk_interior.element -editable yes
    }
    pack $itk_component(element) -expand yes -fill both
    bind $itk_component(element) <<Value>> [itcl::code $this _newValue]

    if { [llength $inactive] > 0 } {
        $itk_component(element) element inactive $inactive
    }
    if { [llength $active] > 0 } {
        $itk_component(element) element active $active
    }
    if { $defval != "" } {
        $itk_component(element) value $defval
    }
    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::PeriodicElementEntry::destructor {} {
    $_owner notify remove $this
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
itcl::body Rappture::PeriodicElementEntry::value {args} {
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
        $itk_component(element) value $newval
    } elseif {[llength $args] != 0} {
        error "wrong # args: should be \"value ?-check? ?newval?\""
    }

    #
    # Query the value and return.
    #
    set how [string trim [$_owner xml get $_path.returnvalue]]
    switch -- $how {
        weight - number - name - symbol - all {
            set how "-$how"
        }
        default {
            set how "-name"
        }
    }
    set str [$itk_component(element) element get $how]
    return $str
}

# ----------------------------------------------------------------------
# USAGE: label
#
# Clients use this to query the label associated with this widget.
# Reaches into the XML and pulls out the appropriate label string.
# ----------------------------------------------------------------------
itcl::body Rappture::PeriodicElementEntry::label {} {
    set label [string trim [$_owner xml get $_path.about.label]]
    if {"" == $label} {
        set label "Element"
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
itcl::body Rappture::PeriodicElementEntry::tooltip {} {
    # query tooltip on-demand based on current element
    return "@[itcl::code $this _tooltip]"
}

# ----------------------------------------------------------------------
# USAGE: _newValue
#
# Invoked automatically whenever the value in the element changes.
# Sends a <<Value>> event to notify clients of the change.
# ----------------------------------------------------------------------
itcl::body Rappture::PeriodicElementEntry::_newValue {} {
    event generate $itk_component(hull) <<Value>>
}

# ----------------------------------------------------------------------
# USAGE: _tooltip
#
# Returns the tooltip for this widget, given the current element in
# the selector.  This is normally called by the Rappture::Tooltip
# facility whenever it is about to pop up a tooltip for this widget.
# ----------------------------------------------------------------------
itcl::body Rappture::PeriodicElementEntry::_tooltip {} {
    set tip [string trim [$_owner xml get $_path.about.description]]

    # get the description for the current element, if there is one
    set str [$itk_component(element) element get -all]
    if {$_path != ""} {
        set desc [string trim [$_owner xml get $_path.about.description]]
    }

    if {[string length $str] > 0 && [string length $desc] > 0} {
        append tip "\n\n$str:\n$desc"
    }
    return $tip
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -state
# ----------------------------------------------------------------------
itcl::configbody Rappture::PeriodicElementEntry::state {
    set valid {normal disabled}
    if {[lsearch -exact $valid $itk_option(-state)] < 0} {
        error "bad value \"$itk_option(-state)\": should be [join $valid {, }]"
    }
    $itk_component(element) configure -state $itk_option(-state)
}
