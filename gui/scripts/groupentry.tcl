# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: GroupEntry - widget containing a group of controls
#
#  This widget represents a <group> entry on a control panel.
#  It contains a series of other controls.  Sort of a glorified
#  frame widget.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

option add *GroupEntry.headingBackground #b5b5b5 widgetDefault
option add *GroupEntry.headingForeground white widgetDefault
option add *GroupEntry.font -*-helvetica-medium-r-normal-*-12-* widgetDefault

itcl::class Rappture::GroupEntry {
    inherit itk::Widget

    itk_option define -heading heading Heading 1
    itk_option define -state state State "normal"

    constructor {owner path args} { # defined below }

    public method value {args}

    public method label {}
    public method tooltip {}

    protected method _fixheading {}

    private variable _owner ""    ;# thing managing this control
    private variable _path ""     ;# path in XML to this number
}

itk::usual GroupEntry {
    keep -cursor -font
    keep -foreground -background
    keep -textbackground
    keep -selectbackground -selectforeground -selectborderwidth
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::GroupEntry::constructor {owner path args} {
    if {[catch {$owner isa Rappture::ControlOwner} valid] != 0 || !$valid} {
        error "bad object \"$owner\": should be Rappture::ControlOwner"
    }
    set _owner $owner
    set _path $path

    itk_component add heading {
        ::label $itk_interior.heading -anchor w
    } {
        usual
        rename -background -headingbackground headingBackground Background
        rename -foreground -headingforeground headingForeground Foreground
    }

    $itk_component(heading) configure \
        -text [string trim [$_owner xml get $_path.about.label]]
    Rappture::Tooltip::for $itk_component(heading) \
        [string trim [$_owner xml get $_path.about.description]]

    itk_component add outline {
        frame $itk_interior.outline -borderwidth 1
    } {
        usual
        ignore -borderwidth
        rename -background -headingbackground headingBackground Background
    }
    pack $itk_component(outline) -expand yes -fill both

    itk_component add inner {
        frame $itk_component(outline).inner -borderwidth 3
    } {
        usual
        ignore -borderwidth
    }
    pack $itk_component(inner) -expand yes -fill both

    eval itk_initialize $args
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
itcl::body Rappture::GroupEntry::value {args} {
    # groups have no value
    return ""
}

# ----------------------------------------------------------------------
# USAGE: label
#
# Clients use this to query the label associated with this widget.
# Reaches into the XML and pulls out the appropriate label string.
# ----------------------------------------------------------------------
itcl::body Rappture::GroupEntry::label {} {
    return ""  ;# manage the label inside this group
}

# ----------------------------------------------------------------------
# USAGE: tooltip
#
# Clients use this to query the tooltip associated with this widget.
# Reaches into the XML and pulls out the appropriate description
# string.  Returns the string that should be used with the
# Rappture::Tooltip facility.
# ----------------------------------------------------------------------
itcl::body Rappture::GroupEntry::tooltip {} {
    return [string trim [$_owner xml get $_path.about.description]]
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -heading
# Turns the heading bar at the top of this group on/off.
# ----------------------------------------------------------------------
itcl::configbody Rappture::GroupEntry::heading {
    if {![string is boolean -strict $itk_option(-heading)]} {
        error "bad value \"$itk_option(-heading)\": should be boolean"
    }

    set str [$itk_component(heading) cget -text]
    if {$itk_option(-heading) && "" != $str} {
        eval pack forget [pack slaves $itk_component(hull)]
        pack $itk_component(heading) -side top -fill x
        pack $itk_component(outline) -expand yes -fill both
        $itk_component(outline) configure -borderwidth 1
        $itk_component(inner) configure -borderwidth 3
    } else {
        pack forget $itk_component(heading)
        $itk_component(outline) configure -borderwidth 0
        $itk_component(inner) configure -borderwidth 0
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -state
# ----------------------------------------------------------------------
itcl::configbody Rappture::GroupEntry::state {
    set valid {normal disabled}
    if {[lsearch -exact $valid $itk_option(-state)] < 0} {
        error "bad value \"$itk_option(-state)\": should be [join $valid {, }]"
    }
}
