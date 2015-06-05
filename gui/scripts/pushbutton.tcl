# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: PushButton - widget for entering a choice of strings
#
#  This widget represents a <choice> entry on a control panel.
#  It is used to choose one of several mutually-exclusive strings.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

option add *PushButton.width 16 widgetDefault
option add *PushButton.height 16 widgetDefault

itcl::class Rappture::PushButton {
    inherit itk::Widget

    itk_option define -variable variable Variable "normal"
    itk_option define -command command Command "normal"
    itk_option define -width width Width "normal"
    itk_option define -height height Height "normal"
    itk_option define -onvalue onValue OnValue "normal"
    itk_option define -offvalue offValue OffValue "normal"
    itk_option define -onbackground onBackground OnBackground "white"
    itk_option define -offbackground offBackground OffBackground "grey85"

    constructor {args} {
        # defined below
    }
    destructor {
        # defined below
    }

    public method invoke {}
    public method deselect {}
    public method select {}
    public method toggle {}
    public method disable {}
    public method enable {}

    protected method _fixValue {args}

    private variable _state 0
    private variable _enabled 1
    public variable command "";         # Command to be invoked
    private variable _variable "";      # Variable to be set.
    public variable onimage "";         # Image displayed when selected
    public variable offimage "";        # Image displayed when deselected.
    public variable disabledimage "";   # Image displayed when deselected.
    public variable onvalue "1";        # Value set when selected.
    public variable offvalue "0";       # Value set when deselected.
    public variable onbackground "white"
    public variable offbackground "grey85"
}

itk::usual PushButton {
    keep -cursor -font
    keep -foreground -background
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::PushButton::constructor {args} {
    itk_component add button {
        label $itk_interior.button -borderwidth 1 -relief sunken
    } {
        usual
        ignore -padx -pady -relief -borderwidth -background
    }
    bind $itk_component(button) <ButtonPress> \
        [itcl::code $this invoke]
    pack $itk_component(button) -expand yes -fill both -ipadx 1 -ipady 1
    eval itk_initialize $args
    deselect
}

itcl::body Rappture::PushButton::invoke {} {
    if { !$_enabled } {
        return
    }
    toggle
    if { $command != "" } {
        uplevel \#0 $command
    }
}

itcl::body Rappture::PushButton::toggle {} {
    if { !$_enabled } {
        return
    }
    set _state [expr !$_state]
    if { $_state } {
        select
    } else {
        deselect
    }
}

itcl::body Rappture::PushButton::disable {} {
    if { $_enabled } {
        set _enabled [expr !$_enabled]
        $itk_component(button) configure -relief raise \
            -image $disabledimage -bg grey85
    }
}

itcl::body Rappture::PushButton::enable {} {
    if { !$_enabled } {
        set _enabled [expr !$_enabled]
        _fixValue
    }
}

# ----------------------------------------------------------------------
# USAGE: _fixValue ?<name1> <name2> <op>?
#
# Invoked automatically whenever the -variable associated with this
# widget is modified.  Copies the value to the current settings for
# the widget.
# ----------------------------------------------------------------------
itcl::body Rappture::PushButton::_fixValue {args} {
    if {"" == $itk_option(-variable)} {
        return
    }
    upvar #0 $itk_option(-variable) var
    if { $var != "" && [string is boolean $var] } {
        set var [expr "$var == 1"]
    }
    if { $var == $onvalue } {
        set _state 1
        $itk_component(button) configure -relief sunken \
            -image $onimage -bg $onbackground
    } else {
        set _state 0
        $itk_component(button) configure -relief raise \
            -image $offimage -bg $offbackground
    }
}

itcl::body Rappture::PushButton::select {} {
    if { !$_enabled } {
        return
    }
    upvar #0 $_variable state
    set state $onvalue
}

itcl::body Rappture::PushButton::deselect {} {
    if { !$_enabled } {
        return
    }
    upvar #0 $_variable state
    set state $offvalue
}


# ----------------------------------------------------------------------
# CONFIGURE: -variable
# ----------------------------------------------------------------------
itcl::configbody Rappture::PushButton::variable {
    if {"" != $_variable} {
        upvar #0 $_variable var
        trace remove variable var write [itcl::code $this _fixValue]
    }
    set _variable $itk_option(-variable)

    if {"" != $_variable} {
        upvar #0 $_variable var
        trace add variable var write [itcl::code $this _fixValue]

        # sync to the current value of this variable
        if {[info exists var]} {
            _fixValue
        }
    }
}

# ----------------------------------------------------------------------
# CONFIGURE: -width
# ----------------------------------------------------------------------
itcl::configbody Rappture::PushButton::width {
    $itk_component(button) configure -width $itk_option(-width)
}

# ----------------------------------------------------------------------
# CONFIGURE: -height
# ----------------------------------------------------------------------
itcl::configbody Rappture::PushButton::height {
    set _height $itk_option(-height)
    $itk_component(button) configure -height $itk_option(-height)
}

