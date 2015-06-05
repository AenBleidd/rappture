# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: animover - animated move
#
#  This widget shows an animation of movement, to show that something
#  is happening when files are being copied or remote machines are
#  being queried.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

option add *Animover.width 200 widgetDefault
option add *Animover.direction both widgetDefault
option add *Animover.delta 5 widgetDefault
option add *Animover.delay 50 widgetDefault

itcl::class Rappture::Animover {
    inherit itk::Widget

    itk_option define -leftimage leftImage LeftImage ""
    itk_option define -rightimage rightImage RightImage ""
    itk_option define -middleimage middleImage MiddleImage ""
    itk_option define -direction direction Direction ""
    itk_option define -delta delta Delta ""
    itk_option define -delay delay Delay 0
    itk_option define -running running Running "disabled"

    constructor {args} { # defined below }

    protected method _redraw {}
    protected variable _x0 0   ;# min value for middle image
    protected variable _x1 0   ;# max value for middle image
    protected variable _x  -1  ;# current position between _x0/_x1
    protected variable _dx 0   ;# delta for x when anim is running

    protected method _animate {}

    #
    # Load various images used as backgrounds for the map
    #
    set dir [file dirname [info script]]
    private common images
    set images(bgl) [image create photo -file \
        [file join $dir images bgl.gif]]
    set images(bgr) [image create photo -file \
        [file join $dir images bgr.gif]]
}

itk::usual Animover {
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Animover::constructor {args} {
    itk_component add area {
        canvas $itk_interior.area \
            -height [expr {[image height $images(bgl)]+4}]
    } {
        usual
        keep -width
    }
    pack $itk_component(area) -expand yes -fill both
    bind $itk_component(area) <Configure> [itcl::code $this _redraw]

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# USAGE: _redraw
#
# Called automatically whenever the widget changes size to redraw
# all elements within it.
# ----------------------------------------------------------------------
itcl::body Rappture::Animover::_redraw {} {
    set c $itk_component(area)
    $c delete all

    set w [winfo width $c]
    set h [winfo height $c]
    set hmid [expr {$h/2}]

    $c create rectangle 2 0 [expr {$w-2}] $h -outline "" -fill #E5CFA1
    $c create image 0 $hmid -anchor w -image $images(bgl)
    $c create image $w $hmid -anchor e -image $images(bgr)

    $c create line 0 2 $w 2 -width 2 -fill black
    $c create line 0 [expr {$h-2}] $w [expr {$h-2}] -width 2 -fill black
    $c create line 20 $hmid [expr {$w-20}] $hmid -width 2 -fill #82765B

    set midw 0
    if {$itk_option(-middleimage) != ""} {
        set midw [expr {[image width $itk_option(-middleimage)]/2}]
    }

    if {$itk_option(-leftimage) != ""} {
        $c create image 4 $hmid -anchor w \
            -image $itk_option(-leftimage)
        set _x0 [expr {4+[image width $itk_option(-leftimage)]+$midw}]
    } else {
        set _x0 [expr {4+$midw}]
    }
    if {$_x0 < 0} { set _x0 0 }

    if {$itk_option(-rightimage) != ""} {
        $c create image [expr {$w-4}] $hmid -anchor e \
            -image $itk_option(-rightimage)
        set _x1 [expr {$w-4-[image width $itk_option(-rightimage)]-$midw}]
    } else {
        set _x1 [expr {$w-4-$midw}]
    }

    if {$_x >= 0} {
        if {$_x < $_x0} {
            set _x $_x0
        } elseif {$_x > $_x1} {
            set _x $_x1
        }
        if {$itk_option(-middleimage) != ""} {
            $c create image $_x $hmid -anchor c \
                -image $itk_option(-middleimage) -tags middle
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _animate
#
# Called periodically when an animation is in progress to update
# the position of the middle element.  If the element reaches the
# left or right side, then it starts over on the other side or
# going the other direction.
# ----------------------------------------------------------------------
itcl::body Rappture::Animover::_animate {} {
    if {$_x >= 0 && $_x0 < $_x1} {
        if {$_x+$_dx <= $_x0} {
            if {$itk_option(-direction) == "left"} {
                set _x $_x1
            } elseif {$itk_option(-direction) == "both"} {
                set _dx [expr {-$_dx}]
                set _x [expr {$_x+$_dx}]
            }
        } elseif {$_x+$_dx >= $_x1} {
            if {$itk_option(-direction) == "right"} {
                set _x $_x0
            } elseif {$itk_option(-direction) == "both"} {
                set _dx [expr {-$_dx}]
                set _x [expr {$_x+$_dx}]
            }
        } else {
            set _x [expr {$_x+$_dx}]
        }

        set c $itk_component(area)
        set h [winfo height $c]
        set hmid [expr {$h/2}]

        if {[$c find withtag middle] == ""} {
            $c create image $_x $hmid -anchor c \
                -image $itk_option(-middleimage) -tags middle
        } else {
            $c coords middle $_x $hmid
        }
    }
    if {$_x >= 0} {
        after $itk_option(-delay) [itcl::code $this _animate]
    }
}

# ----------------------------------------------------------------------
# OPTION: -running
# Used to start/stop the animation.
# ----------------------------------------------------------------------
itcl::configbody Rappture::Animover::running {
    switch -- $itk_option(-running) {
        normal {
            if {$_x < 0} {
                set _x $_x0
                after idle [itcl::code $this _animate]
            }
        }
        disabled {
            if {$_x > 0} {
                set _x -1
                after cancel [itcl::code $this _animate]
                $itk_component(area) delete middle
            }
        }
        default {
            error "bad value \"$itk_option(-running)\": should be normal or disabled"
        }
    }
}

# ----------------------------------------------------------------------
# OPTION: -direction
# Used to control the direction of movement for the animation.
# ----------------------------------------------------------------------
itcl::configbody Rappture::Animover::direction {
    switch -- $itk_option(-direction) {
        left  { set _dx [expr {-1*$itk_option(-delta)}] }
        right { set _dx $itk_option(-delta) }
        both  { set _dx $itk_option(-delta) }
        default {
            error "bad value \"$itk_option(-direction)\": should be left, right, or both"
        }
    }
}

# ----------------------------------------------------------------------
# OPTION: -delta
# Used to control the amount of movement for the animation.
# ----------------------------------------------------------------------
itcl::configbody Rappture::Animover::delta {
    if {$itk_option(-delta) < 1} {
        error "bad value \"$itk_option(-delta)\": should be int >= 1"
    }
}
