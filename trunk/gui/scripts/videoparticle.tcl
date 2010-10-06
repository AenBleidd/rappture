# ----------------------------------------------------------------------
#  COMPONENT: videoparticle - mark a particle on a video frame
#
# ======================================================================
#  AUTHOR:  Derrick Kearney, Purdue University
#  Copyright (c) 2005-2010  Purdue Research Foundation
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
    itk_option define -name name Name ""
    itk_option define -color color Color "green"
    itk_option define -fncallback fncallback Fncallback ""
    itk_option define -trajcallback trajcallback Trajcallback ""

    constructor { args } {
        # defined below
    }
    destructor {
        # defined below
    }

    public method Add {args}
    public method Delete {args}
    public method Show {args}
    public method Hide {args}
    public method Link {args}
    public method Coords {}
    public method Frame {}

    public variable  fncallback ""      ;# framenumber callback - tells what frame we are on
    public variable  trajcallback ""    ;# trajectory callback - calculates and draws trajectory

    public method Move {status x y}
    public method Menu {args}

    public method drawVectors {args}
    public method next {args}
    public method prev {args}

    private variable _canvas        ""  ;# canvas which owns the particle
    private variable _name          ""  ;# id of the particle
    private variable _color         ""  ;# color of the particle
    private variable _framexy       ""  ;# list of frame numbers and coords
                                        ;# where the particle lives
    private variable _halo           0  ;# about the diameter of the particle
    private variable _menu          ""  ;# particle controls balloon widget
    private variable _nextnode      ""  ;# particle this particle points to
    private variable _prevnode      ""  ;# particle this particle is pointed to by
    private variable _link          ""  ;# tag of vector linking this and nextnode
}

itk::usual VideoParticle {
    keep -background -foreground -cursor -font
    keep -plotbackground -plotforeground
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::VideoParticle::constructor {win args} {

    # setup the particle control menu
    set _menu $itk_component(hull).particlecontrols
    Rappture::Balloon ${_menu} -title "Particle Controls"
    set controls [${_menu} component inner]

    # Link control
    #button $controls.link -text Link \
    #    -relief flat -pady 0 -padx 0  -font "Arial 9" \
    #    -command [itcl::code $this Link]  -overrelief flat \
    #    -activebackground grey90

    # Delete control
    button $controls.delete -text Delete \
        -relief flat -pady 0 -padx 0  -font "Arial 9" \
        -command [itcl::code $this Delete frame]  -overrelief flat \
        -activebackground grey90

    #grid $controls.link       -column 0 -row 0 -sticky w
    grid $controls.delete     -column 0 -row 1 -sticky w
    # grid $controls.rename     -column 0 -row 2 -sticky w
    # grid $controls.recolor    -column 0 -row 3 -sticky w

    grid columnconfigure $controls 0  -weight 1

    set _canvas $win

    # finish configuring the particle
    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::VideoParticle::destructor {} {
}

# ----------------------------------------------------------------------
# Add - add attributes to the particle
#   frame <frameNum> <x> <y> - add a frame and location
# ----------------------------------------------------------------------
itcl::body Rappture::VideoParticle::Add {args} {
    set option [lindex $args 0]
    switch -- $option {
        "frame" {
            if {[llength $args] == 4} {
                foreach { frNum x y } [lrange $args 1 end] break
                if {([string is integer $frNum] != 1)} {
                    error "bad value: \"$frNum\": frame number should be an integer"
                }
                if {([string is double $x] != 1)} {
                    error "bad value: \"$frNum\": x coordinate should be a number"
                }
                if {([string is double $y] != 1)} {
                    error "bad value: \"$frNum\": y coordinate should be a number"
                }
                # if the particle is alread in the frame,
                # update it's x,y coords. othrewise add it.
                set idx [lsearch ${_framexy} $frNum]
                if {$idx == -1} {
                    lappend _framexy $frNum [list $x $y]
                } else {
                    incr idx
                    set _framexy [lreplace ${_framexy} $idx $idx [list $x $y]]
                }
            } else {
                error "wrong # args: should be \"frame <frameNumber> <x> <y>\""
            }
        }
        default {
            error "bad option \"$option\": should be frame."
        }
    }
}

# ----------------------------------------------------------------------
# Delete - remove the particle
# ----------------------------------------------------------------------
itcl::body Rappture::VideoParticle::Delete {args} {

    Menu deactivate

    set _framexy ""
    Hide particle

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
}

# ----------------------------------------------------------------------
# Show - draw the particle
#   particle - draw the particle on the canvas
#   name - popup a ballon with the name of this object
# ----------------------------------------------------------------------
itcl::body Rappture::VideoParticle::Show {args} {
    set option [lindex $args 0]
    switch -- $option {
        "particle" {
            foreach {x y} [lindex ${_framexy} 1] break
            set coords [list [expr $x-${_halo}] [expr $y-${_halo}] \
                             [expr $x+${_halo}] [expr $y+${_halo}]]
            ${_canvas} create oval $coords \
                -fill ${_color} \
                -width 0 \
                -tags "particle ${_name}"
        }
        name {

        }
        default {
            error "bad option \"$option\": should be \"frame <frameNumber>\"."
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
        "particle" {
            if {[llength $args] != 1} {
                error "wrong # args: should be \"particle\""
            }
            puts "hiding ${_name}"
            ${_canvas} delete "${_name}"
        }
        name {

        }
        default {
            error "bad option \"$option\": should be \"particle or name\"."
        }
    }
}

# ----------------------------------------------------------------------
# Move - move the particle to a new location
# ----------------------------------------------------------------------
itcl::body Rappture::VideoParticle::Move {status x y} {
    switch -- $status {
        "press" {
        }
        "motion" {
        }
        "release" {
        }
        default {
            error "bad option \"$option\": should be one of press, motion, release."
        }
    }
    set coords [list [expr $x-${_halo}] [expr $y-${_halo}] \
                     [expr $x+${_halo}] [expr $y+${_halo}]]
    eval ${_canvas} coords ${_name} $coords
    set _framexy [lreplace ${_framexy} 1 1 [list $x $y]]
    drawVectors
    if {[string compare "" ${_prevnode}] != 0} {
        ${_prevnode} drawVectors
    }
}

# ----------------------------------------------------------------------
# Menu - popup a menu with the particle controls
#   create
#   activate x y
#   deactivate
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
            ${_menu} activate @$x,$y $dir
        }
        "deactivate" {
            ${_menu} deactivate
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
itcl::body Rappture::VideoParticle::drawVectors {args} {

    if {[string compare "" $trajcallback] != 0} {
        uplevel \#0 $trajcallback $this ${_nextnode}
    }
}


# ----------------------------------------------------------------------
# Coords - return the x and y coordinates as a list
#          for the first frame this particle appears in
# ----------------------------------------------------------------------
itcl::body Rappture::VideoParticle::Coords {} {
    return [lindex ${_framexy} 1]
}

# ----------------------------------------------------------------------
# Frame - return the frame this particle appears in
# ----------------------------------------------------------------------
itcl::body Rappture::VideoParticle::Frame {} {
    return [lindex ${_framexy} 0]
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
# CONFIGURATION OPTION: -name
# ----------------------------------------------------------------------
itcl::configbody Rappture::VideoParticle::name {
    set _name $itk_option(-name)
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
