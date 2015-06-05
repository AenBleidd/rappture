# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: animicon - an animated icon
#
#  This widget displays an animated icon.  It acts like a label, but
#  it allows you to start/stop the animation.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

option add *Animicon.delay 100 widgetDefault
option add *Animicon.textBackground white widgetDefault

itcl::class Rappture::Animicon {
    inherit itk::Widget

    itk_option define -delay delay Delay 100
    itk_option define -images images Images ""

    constructor {args} { # defined below }

    public method start {}
    public method stop {}
    public method isrunning {}

    protected method _next {}
    protected method _loadFrames {}

    private variable _frames      ;# array of frames indexed from 0
    private variable _pos 0       ;# current position in anim sequence
    private variable _afterid ""  ;# after ID for next animation step
}

itk::usual Animicon {
    keep -cursor -background
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Animicon::constructor {args} {
    itk_component add icon {
        label $itk_interior.icon
    }
    pack $itk_component(icon) -expand yes -fill both

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# USAGE: start
#
# Clients use this to start the animation.  If the animation is
# already running, it does nothing.  Otherwise, it starts the
# animation.
# ----------------------------------------------------------------------
itcl::body Rappture::Animicon::start {} {
    if {![isrunning]} {
        _next
    }
}

# ----------------------------------------------------------------------
# USAGE: stop
#
# Clients use this to stop the animation.  If the animation is
# not running, it does nothing.  Otherwise, it stops the
# animation on the current frame.
# ----------------------------------------------------------------------
itcl::body Rappture::Animicon::stop {} {
    if {[isrunning]} {
        after cancel $_afterid
        set _afterid ""
    }
}

# ----------------------------------------------------------------------
# USAGE: isrunning
#
# Returns true if the animation is currently running, and false
# otherwise.
# ----------------------------------------------------------------------
itcl::body Rappture::Animicon::isrunning {} {
    return [expr {"" != $_afterid}]
}

# ----------------------------------------------------------------------
# USAGE: _next
#
# Used internally to load the next animation frame.
# ----------------------------------------------------------------------
itcl::body Rappture::Animicon::_next {} {
    $itk_component(icon) configure -image $_frames($_pos)
    if {[incr _pos] >= [array size _frames]} {
        set _pos 0
    }
    set _afterid [after $itk_option(-delay) [itcl::code $this _next]]
}

# ----------------------------------------------------------------------
# OPTION: -images
#
# Sets a sequence of icon names that will be loaded for an animation.
# ----------------------------------------------------------------------
itcl::configbody Rappture::Animicon::images {
    set restart [isrunning]
    stop

    array unset _frames
    if {[llength $itk_option(-images)] >= 1} {
        set w 0
        set h 0
        set i 0
        foreach name $itk_option(-images) {
            set imh [Rappture::icon $name]
            if {"" == $imh} {
                error "image not found: $name"
            }
            set _frames($i) $imh
            if {[image width $imh] > $w} { set w [image width $imh] }
            if {[image height $imh] > $h} { set h [image height $imh] }
            incr i
        }
    }

    $itk_component(icon) configure -width $w -height $h -image $_frames(0)
    set _pos 0

    if {$restart} {
        start
    }
}
