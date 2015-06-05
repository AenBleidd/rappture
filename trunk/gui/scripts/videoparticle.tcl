# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: videoparticle - mark a particle on a video frame
#
# ======================================================================
#  AUTHOR:  Derrick Kearney, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
# See the file "license.terms" for information on usage and redistribution of
# this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require BLT
package require Img
package require Rappture
package require RapptureGUI

itcl::class Rappture::VideoParticle {
    inherit itk::Widget

    itk_option define -halo halo Halo "10"
    itk_option define -color color Color "green"
    itk_option define -fncallback fncallback Fncallback ""
    itk_option define -bindentercb bindentercb Bindentercb ""
    itk_option define -bindleavecb bindleavecb Bindleavecb ""
    itk_option define -trajcallback trajcallback Trajcallback ""
    itk_option define -px2dist px2dist Px2dist ""
    itk_option define -units units Units "m/s"
    itk_option define -bindings bindings Bindings "enable"
    itk_option define -ondelete ondelete Ondelete ""
    itk_option define -onframe onframe Onframe ""
    itk_option define -framerange framerange Framerange ""

    constructor { name win args } {
        # defined below
    }
    destructor {
        # defined below
    }

    public method Show {args}
    public method Hide {args}
    public method Link {args}
    public method Coords {args}
    public method Frame {args}
    public method drawVectors {}
    public method next {args}
    public method prev {args}
    public method name {}

    public variable  fncallback ""      ;# framenumber callback - tells what frame we are on
    public variable  bindentercb ""     ;# enter binding callback - call this when entering the object
    public variable  bindleavecb ""     ;# leave binding callback - call this when leaving the object
    public variable  trajcallback ""    ;# trajectory callback - calculates and draws trajectory

    public method Move {status x y}
    public method Menu {args}

    protected method Enter {}
    protected method Leave {}
    protected method CatchEvent {event}

    protected method _fixValue {args}
    protected method _fixPx2Dist {px2dist}
    protected method _fixBindings {status}

    private variable _canvas        ""  ;# canvas which owns the particle
    private variable _name          ""  ;# id of the particle
    private variable _color         ""  ;# color of the particle
    private variable _frame          0  ;# frame number where this object lives
    private variable _coords        ""  ;# list of coords where the object lives
    private variable _halo           0  ;# about the diameter of the particle
    private variable _x              0  ;# x coord when "pressed" for motion
    private variable _y              0  ;# y coord when "pressed" for motion
    private variable _nextnode      ""  ;# particle this particle points to
    private variable _prevnode      ""  ;# particle this particle is pointed to by
    private variable _link          ""  ;# tag of vector linking this and nextnode
    private variable _units         ""  ;#
    private variable _px2dist       ""  ;# variable associated with -px2dist
}

itk::usual VideoParticle {
    keep -background -foreground -cursor -font
    keep -plotbackground -plotforeground
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::VideoParticle::constructor {name win args} {

    set _name $name
    set _canvas $win

    # setup the particle control menu
    itk_component add menu {
        Rappture::Balloon $itk_interior.particlecontrols -title "Particle Controls"
    }

    set controls [$itk_component(menu) component inner]

    # Frame number control
    label $controls.framenuml -text "Frame" -font "Arial 9"\
         -highlightthickness 0
    Rappture::Spinint $controls.framenume \
        -min 0 -width 5 -font "arial 9"

    # Delete control
    label $controls.deletel -text "Delete" -font "Arial 9" \
        -highlightthickness 0
    Rappture::Switch $controls.deleteb -showtext "false"
    $controls.deleteb value false

    # Save button
    button $controls.saveb -text Save \
        -relief raised -pady 0 -padx 0  -font "Arial 9" \
        -command [itcl::code $this Menu deactivate save] \
        -activebackground grey90

    # Cancel button
    button $controls.cancelb -text Cancel \
        -relief raised -pady 0 -padx 0  -font "Arial 9" \
        -command [itcl::code $this Menu deactivate cancel] \
        -activebackground grey90


    grid $controls.framenuml -column 0 -row 0 -sticky e
    grid $controls.framenume -column 1 -row 0 -sticky w
    grid $controls.deletel   -column 0 -row 1 -sticky e
    grid $controls.deleteb   -column 1 -row 1 -sticky w
    grid $controls.saveb     -column 0 -row 2 -sticky e
    grid $controls.cancelb   -column 1 -row 2 -sticky w


    grid columnconfigure $controls 0 -weight 1

    # finish configuring the particle
    eval itk_initialize $args

    # set the frame for the particle
    Frame [uplevel \#0 $fncallback]
    bind ${_name}-FrameEvent <<Frame>> [itcl::code $this CatchEvent Frame]
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::VideoParticle::destructor {} {
    configure -px2dist ""  ;# remove variable trace

    Hide object

    # delete the vectors originating from this particle
    if {[string compare "" ${_nextnode}] != 0} {
        ${_canvas} delete ${_link}
        ${_nextnode} prev ${_prevnode}
    }

    # delete the vectors pointing to this particle
    if {[string compare "" ${_prevnode}] != 0} {
        ${_prevnode} next ${_nextnode}
        ${_prevnode} drawVectors
    }

    _fixBindings disable

    if {"" != $itk_option(-ondelete)} {
        uplevel \#0 $itk_option(-ondelete)
    }
}

# ----------------------------------------------------------------------
#   Enter - bindings if the mouse enters the object's space
# ----------------------------------------------------------------------
itcl::body Rappture::VideoParticle::Enter {} {
    uplevel \#0 $bindentercb
}


# ----------------------------------------------------------------------
#   Leave - bindings if the mouse leaves the object's space
# ----------------------------------------------------------------------
itcl::body Rappture::VideoParticle::Leave {} {
    uplevel \#0 $bindleavecb
}


# ----------------------------------------------------------------------
#   CatchEvent - bindings for caught events
# ----------------------------------------------------------------------
itcl::body Rappture::VideoParticle::CatchEvent {event} {
    switch -- $event {
        "Frame" {
            if {[uplevel \#0 $fncallback] == ${_frame}} {
                ${_canvas} itemconfigure ${_name}-particle -fill red
            } else {
                ${_canvas} itemconfigure ${_name}-particle -fill ${_color}
            }
        }
        default {
            error "bad event \"$event\": should be one of Frame."
        }

    }
}


# ----------------------------------------------------------------------
# Show - draw the particle
#   particle - draw the particle on the canvas
#   name - popup a ballon with the name of this object
# ----------------------------------------------------------------------
itcl::body Rappture::VideoParticle::Show {args} {
    set option [lindex $args 0]
    switch -- $option {
        "object" {
            foreach {x y} ${_coords} break
            set coords [list [expr $x-${_halo}] [expr $y-${_halo}] \
                             [expr $x+${_halo}] [expr $y+${_halo}]]
            ${_canvas} create oval $coords \
                -fill ${_color} \
                -width 0 \
                -tags "particle ${_name} ${_name}-particle"
        }
        "name" {

        }
        default {
            error "bad option \"$option\": should be one of object, name."
        }
    }
}

# ----------------------------------------------------------------------
# Hide
#   particle - remove the particle from where it is drawn
#   name - remove the popup with the name
# ----------------------------------------------------------------------
itcl::body Rappture::VideoParticle::Hide {args} {
    set option [lindex $args 0]
    switch -- $option {
        "object" {
            if {[llength $args] != 1} {
                error "wrong # args: should be \"particle\""
            }
            ${_canvas} delete "${_name}"
        }
        "name" {

        }
        default {
            error "bad option \"$option\": should be one of object, name."
        }
    }
}

# ----------------------------------------------------------------------
# Move - move the object to a new location
# ----------------------------------------------------------------------
itcl::body Rappture::VideoParticle::Move {status x y} {
    switch -- $status {
        "press" {
            set _x $x
            set _y $y
        }
        "motion" {
            ${_canvas} move ${_name} [expr $x-${_x}] [expr $y-${_y}]
            foreach {x0 y0 x1 y1} [${_canvas} coords ${_name}-particle] break
            set _coords [list [expr $x0+${_halo}] [expr $y0+${_halo}]]
            set _x $x
            set _y $y
            drawVectors
            if {[string compare "" ${_prevnode}] != 0} {
                ${_prevnode} drawVectors
            }
        }
        "release" {
        }
        default {
            error "bad option \"$option\": should be one of press, motion, release."
        }
    }
}

# ----------------------------------------------------------------------
# Menu - popup a menu with the particle controls
#   create
#   activate x y
#   deactivate status
# ----------------------------------------------------------------------
itcl::body Rappture::VideoParticle::Menu {args} {
    set option [lindex $args 0]
    switch -- $option {
        "activate" {
            if {[llength $args] != 3} {
                error "wrong # args: should be \"activate <x> <y>\""
            }
            foreach {x y} [lrange $args 1 end] break
            set dir "left"
            set x0 [winfo rootx ${_canvas}]
            set y0 [winfo rooty ${_canvas}]
            set w0 [winfo width ${_canvas}]
            set h0 [winfo height ${_canvas}]
            set x [expr $x0+$x]
            set y [expr $y0+$y]
            $itk_component(menu) activate @$x,$y $dir

            # update the values in the menu
            set controls [$itk_component(menu) component inner]
            $controls.framenume value ${_frame}
            $controls.deleteb value false
        }
        "deactivate" {
            $itk_component(menu) deactivate
            if {[llength $args] != 2} {
                error "wrong # args: should be \"deactivate <status>\""
            }
            set status [lindex $args 1]
            switch -- $status {
                "save" {
                    set controls [$itk_component(menu) component inner]

                    set newframenum [$controls.framenume value]
                    if {${_frame} != $newframenum} {
                        Frame $newframenum
                    }

                    if {[$controls.deleteb value]} {
                        itcl::delete object $this
                    }
                }
                "cancel" {
                }
                "default" {
                    error "bad value \"$status\": should be one of save, cancel"
                }
            }
        }
        default {
            error "bad option \"$option\": should be one of activate, deactivate."
        }
    }
}

# ----------------------------------------------------------------------
# Link - move the particle to a new location
# ----------------------------------------------------------------------
itcl::body Rappture::VideoParticle::Link {args} {
    # add a new particle list of linked particles
    foreach {p} $args break
    $p prev $this
    next $p
    drawVectors
}

# ----------------------------------------------------------------------
# drawVectors - draw vectors from this particle
#               to all particles it is linked to.
# ----------------------------------------------------------------------
itcl::body Rappture::VideoParticle::drawVectors {} {

    if {[string compare "" $trajcallback] != 0} {
        set _link [uplevel \#0 $trajcallback $this ${_nextnode}]
    }
}


# ----------------------------------------------------------------------
#   Coords ?<x0> <y0>? - update the coordinates of this object
# ----------------------------------------------------------------------
itcl::body Rappture::VideoParticle::Coords {args} {
    if {[llength $args] == 0} {
        return ${_coords}
    } elseif {[llength $args] == 1} {
        foreach {x0 y0} [lindex $args 0] break
    } elseif {[llength $args] == 2} {
        foreach {x0 y0} $args break
    } else {
        error "wrong # args: should be \"Coords ?<x0> <y0>?\""
    }

    if {([string is double $x0] != 1)} {
        error "bad value: \"$x0\": x coordinate should be a double"
    }
    if {([string is double $y0] != 1)} {
        error "bad value: \"$y0\": y coordinate should be a double"
    }

    set _coords [list $x0 $y0]
    set coords [list [expr $x0-${_halo}] [expr $y0-${_halo}] \
                     [expr $x0+${_halo}] [expr $y0+${_halo}]]

    if {[llength [${_canvas} find withtag ${_name}-particle]] > 0} {
        eval ${_canvas} coords ${_name}-particle $coords
    }

    _fixValue
    return ${_coords}
}

# ----------------------------------------------------------------------
#   Frame ?<frameNum>? - update the frame this object is in
# ----------------------------------------------------------------------
itcl::body Rappture::VideoParticle::Frame {args} {
    if {[llength $args] == 1} {
        set val [lindex $args 0]
        if {([string is integer $val] != 1)} {
            error "bad value: \"$val\": frame number should be an integer"
        }

        set _frame $val

        if {"" != $itk_option(-onframe)} {
            uplevel \#0 $itk_option(-onframe) ${_frame}
        }

        drawVectors
        if {[string compare "" ${_prevnode}] != 0} {
            ${_prevnode} drawVectors
        }
    } elseif {[llength $args] != 0} {
        error "wrong # args: should be \"Frame ?<number>?\""
    }
    return ${_frame}
}


# ----------------------------------------------------------------------
# next - get/set the next particle
# ----------------------------------------------------------------------
itcl::body Rappture::VideoParticle::next {args} {
    if {[llength $args] == 1} {
        # set the next node
        set _nextnode [lindex $args 0]
        # drawVectors
    }
    return ${_nextnode}
}

# ----------------------------------------------------------------------
# prev - get/set the prev particle
# ----------------------------------------------------------------------
itcl::body Rappture::VideoParticle::prev {args} {
    if {[llength $args] == 1} {
        # set the prev node
        set _prevnode [lindex $args 0]
    }
    return ${_prevnode}
}

# ----------------------------------------------------------------------
# name - get the name of the particle
# ----------------------------------------------------------------------
itcl::body Rappture::VideoParticle::name {} {
    return ${_name}
}

# ----------------------------------------------------------------------
# USAGE: _fixValue
# Invoked automatically whenever the -px2dist associated with this
# widget is modified.  Copies the value to the current settings for
# the widget.
# ----------------------------------------------------------------------
itcl::body Rappture::VideoParticle::_fixValue {args} {
    if {"" == $itk_option(-px2dist)} {
        return
    }
    upvar #0 $itk_option(-px2dist) var

    drawVectors
}

# ----------------------------------------------------------------------
# USAGE: _fixPx2Dist
# Invoked whenever the length part of the trajectory for this object
# is changed by the user via the popup menu.
# ----------------------------------------------------------------------
itcl::body Rappture::VideoParticle::_fixPx2Dist {px2dist} {
    if {"" == $itk_option(-px2dist)} {
        return
    }
    upvar #0 $itk_option(-px2dist) var
    set var $px2dist
}

# ----------------------------------------------------------------------
# _fixBindings - enable/disable bindings
#   enable
#   disable
# ----------------------------------------------------------------------
itcl::body Rappture::VideoParticle::_fixBindings {status} {
    switch -- $status {
        "enable" {
            ${_canvas} bind ${_name} <ButtonPress-1>   [itcl::code $this Move press %x %y]
            ${_canvas} bind ${_name} <B1-Motion>       [itcl::code $this Move motion %x %y]
            ${_canvas} bind ${_name} <ButtonRelease-1> [itcl::code $this Move release %x %y]

            ${_canvas} bind ${_name} <ButtonPress-3>   [itcl::code $this Menu activate %x %y]

            ${_canvas} bind ${_name} <Enter>           [itcl::code $this Enter]
            ${_canvas} bind ${_name} <Leave>           [itcl::code $this Leave]

            ${_canvas} bind ${_name} <B1-Enter>        { }
            ${_canvas} bind ${_name} <B1-Leave>        { }
            # bind ${_canvas} <<Frame>>                  +[itcl::code $this CatchEvent Frame]
            bindtags ${_canvas} [concat ${_name}-FrameEvent [bindtags ${_canvas}]]
        }
        "disable" {
            ${_canvas} bind ${_name} <ButtonPress-1>   { }
            ${_canvas} bind ${_name} <B1-Motion>       { }
            ${_canvas} bind ${_name} <ButtonRelease-1> { }

            ${_canvas} bind ${_name} <ButtonPress-3>   { }

            ${_canvas} bind ${_name} <Enter>           { }
            ${_canvas} bind ${_name} <Leave>           { }

            ${_canvas} bind ${_name} <B1-Enter>        { }
            ${_canvas} bind ${_name} <B1-Leave>        { }
            set tagnum [lsearch [bindtags ${_canvas}] "${_name}-FrameEvent"]
            if {$tagnum >= 0} {
                bindtags ${_canvas} [lreplace [bindtags ${_canvas}] $tagnum $tagnum]
            }
        }
        default {
            error "bad option \"$status\": should be one of enable, disable."
        }
    }
}


# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -halo
# ----------------------------------------------------------------------
itcl::configbody Rappture::VideoParticle::halo {
    if {[string is double $itk_option(-halo)] == 1} {
        set _halo $itk_option(-halo)
    } else {
        error "bad value: \"$itk_option(-halo)\": halo should be a number"
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -color
# ----------------------------------------------------------------------
itcl::configbody Rappture::VideoParticle::color {
    if {[string compare "" $itk_option(-color)] != 0} {
        # FIXME how to tell if the color is valid?
        set _color $itk_option(-color)
    } else {
        error "bad value: \"$itk_option(-color)\": should be a valid color"
    }
}

# ----------------------------------------------------------------------
# CONFIGURE: -px2dist
# ----------------------------------------------------------------------
itcl::configbody Rappture::VideoParticle::px2dist {
    if {"" != $_px2dist} {
        upvar #0 $_px2dist var
        trace remove variable var write [itcl::code $this _fixValue]
    }

    set _px2dist $itk_option(-px2dist)

    if {"" != $_px2dist} {
        upvar #0 $_px2dist var
        trace add variable var write [itcl::code $this _fixValue]

        # sync to the current value of this variable
        if {[info exists var]} {
            _fixValue
        }
    }
}


# ----------------------------------------------------------------------
# CONFIGURE: -units
# ----------------------------------------------------------------------
itcl::configbody Rappture::VideoParticle::units {
    set _units $itk_option(-units)
    # _fixValue
}


# ----------------------------------------------------------------------
# CONFIGURE: -bindings
# ----------------------------------------------------------------------
itcl::configbody Rappture::VideoParticle::bindings {
    _fixBindings $itk_option(-bindings)
}

# ----------------------------------------------------------------------
# CONFIGURE: -framerange
# ----------------------------------------------------------------------
itcl::configbody Rappture::VideoParticle::framerange {
    if {"" == $itk_option(-framerange)} {
        return
    }
    if {[llength $itk_option(-framerange)] != 2} {
        error "bad value \"$itk_option(-framerange)\": should be 2 integers"
    }
    foreach {min max} $itk_option(-framerange) break
    if {!([string is integer $min]) || !([string is integer $max])} {
        error "bad value \"$itk_option(-framerange)\": should be 2 integers"
    }
    set controls [$itk_component(menu) component inner]
    $controls.framenume configure -min $min -max $max
}

# ----------------------------------------------------------------------
