# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: dropdown - base class for drop-down panels
#
#  This is the base class for a family of drop-down widget panels.
#  They might be used, for example, to build the drop-down list for
#  a combobox.
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

option add *Dropdown.textBackground white widgetDefault
option add *Dropdown.outline black widgetDefault
option add *Dropdown.borderwidth 1 widgetDefault
option add *Dropdown.font -*-helvetica-medium-r-normal-*-12-* widgetDefault

itcl::class Rappture::Dropdown {
    inherit itk::Toplevel

    itk_option define -outline outline Outline ""
    itk_option define -postcommand postCommand PostCommand ""
    itk_option define -unpostcommand unpostCommand UnpostCommand ""

    constructor {args} { # defined below }

    public method post {where args}
    public method unpost {}

    protected method _adjust {{widget ""}}

    public proc outside {w x y}

    bind RapptureDropdown <ButtonPress> \
        {if {[Rappture::Dropdown::outside %W %X %Y]} {%W unpost}}
}

itk::usual Dropdown {
    keep -background -outline -cursor -font
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Dropdown::constructor {args} {
    wm overrideredirect $itk_component(hull) yes
    wm withdraw $itk_component(hull)

    component hull configure -borderwidth 1 -background black
    itk_option remove hull.background hull.borderwidth

    # add bindings to release the grab
    set btags [bindtags $itk_component(hull)]
    bindtags $itk_component(hull) [linsert $btags 1 RapptureDropdown]

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# USAGE: post @<x>,<y>
# USAGE: post <widget> <justify>
#
# Clients use this to pop up the dropdown on the screen.  The position
# should be either a specific location "@x,y", or a <widget> and its
# justification.
# ----------------------------------------------------------------------
itcl::body Rappture::Dropdown::post {where args} {
    set owner [expr {([winfo exists $where]) ? $where : ""}]
    _adjust $owner    ;# make sure contents are up-to-date
    update idletasks  ;# fix size info

    if {[string length $itk_option(-postcommand)] > 0} {
        set cmd [list uplevel #0 $itk_option(-postcommand)]
        if {[catch $cmd result]} {
            bgerror $result
        }
    }

    set w [winfo width $itk_component(hull)]
    set h [winfo height $itk_component(hull)]
    set sw [winfo screenwidth $itk_component(hull)]
    set sh [winfo screenheight $itk_component(hull)]

    if {[regexp {^@([0-9]+),([0-9]+)$} $where match x y]} {
        set xpos $x
        set ypos $y
    } elseif {[winfo exists $where]} {
        set x0 [winfo rootx $where]
        switch -- $args {
            left { set xpos $x0 }
            right { set xpos [expr {$x0 + [winfo width $where] - $sw}] }
            default {
                error "bad option \"$args\": should be left, right"
            }
        }
        set ypos [expr {[winfo rooty $where]+[winfo height $where]}]
    } else {
        error "bad position \"$where\": should be widget name or @x,y"
    }

    # make sure the dropdown doesn't go off screen
    if {$xpos > 0} {
        # left-justified positions
        if {$xpos + $w > $sw} {
            set xpos [expr {$sw-$w}]
            if {$xpos < 0} { set xpos 0 }
        }
        set xpos "+$xpos"
    } else {
        # right-justified positions
        if {$xpos - $w < -$sw} {
            set xpos [expr {-$sw+$w}]
            if {$xpos > 0} { set xpos -1 }
        }
    }
    if {$ypos + $h > $sh} {
        set ypos [expr {$sh-$h}]
        if {$ypos < 0} { set ypos 0 }
    }

    # post the dropdown on the screen
    wm geometry $itk_component(hull) "$xpos+$ypos"
    update

    wm deiconify $itk_component(hull)
    raise $itk_component(hull)

    # grab the mouse pointer
    update
    while {[catch {grab set -global $itk_component(hull)}]} {
        after 100
    }
}

# ----------------------------------------------------------------------
# USAGE: unpost
#
# Takes down the dropdown, if it is showing on the screen.
# ----------------------------------------------------------------------
itcl::body Rappture::Dropdown::unpost {} {
    grab release $itk_component(hull)
    wm withdraw $itk_component(hull)

    if {[string length $itk_option(-unpostcommand)] > 0} {
        set cmd [list uplevel #0 $itk_option(-unpostcommand)]
        if {[catch $cmd result]} {
            bgerror $result
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _adjust
#
# This method is invoked each time the dropdown is posted to adjust
# its size and contents.
# ----------------------------------------------------------------------
itcl::body Rappture::Dropdown::_adjust {{widget ""}} {
    # derived classes redefine this to do something useful
}

# ----------------------------------------------------------------------
# USAGE: outside <widget> <x> <y>
#
# Checks to see if the root coordinate <x>,<y> is outside of the
# area for the <widget>.  Returns 1 if so, and 0 otherwise.
# ----------------------------------------------------------------------
itcl::body Rappture::Dropdown::outside {widget x y} {
    return [expr {$x < [winfo rootx $widget]
             || $x > [winfo rootx $widget]+[winfo width $widget]
             || $y < [winfo rooty $widget]
             || $y > [winfo rooty $widget]+[winfo height $widget]}]
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -outline
# ----------------------------------------------------------------------
itcl::configbody Rappture::Dropdown::outline {
    component hull configure -background $itk_option(-outline)
}
