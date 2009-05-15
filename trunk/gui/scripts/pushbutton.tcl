# ----------------------------------------------------------------------
#  COMPONENT: PushButton - widget for entering a choice of strings
#
#  This widget represents a <choice> entry on a control panel.
#  It is used to choose one of several mutually-exclusive strings.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

itcl::class Rappture::PushButton {
    inherit itk::Widget

    itk_option define -variable variable Variable "normal"
    itk_option define -command command Command "normal"

    constructor {args} { # defined below }
    destructor { # defined below }

    public method invoke {}
    public method deselect {}
    public method select {}
    public method toggle {}

    private variable _state 0
    public variable command "";		# Command to be invoked 
    public variable variable "";	# Variable to be set. 
    public variable onimage "";		# Image displayed when selected 
    public variable offimage "";	# Image displayed when deselected. 
    public variable onvalue "1";	# Value set when selected.
    public variable offvalue "0";	# Value set when deselected. 
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
    toggle
    if { $command != "" } {
	uplevel \#0 $command
    }
}

itcl::body Rappture::PushButton::toggle {} {
    set _state [expr !$_state]
    if { $_state } {
	select
    } else {
	deselect
    }
}

itcl::body Rappture::PushButton::select {} {
    set _state 1
    $itk_component(button) configure -relief sunken \
	-image $onimage -bg white
    upvar #0 $variable state
    set state $onvalue
}

itcl::body Rappture::PushButton::deselect {} {
    set _state 0
    $itk_component(button) configure -relief raise \
	-image $offimage -bg grey85
    upvar #0 $variable state
    set state $offvalue
}
