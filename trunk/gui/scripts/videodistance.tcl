# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: videodistance - specify a distance in a video canvas
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

itcl::class Rappture::VideoDistance {
    inherit itk::Widget

    itk_option define -color color Color "green"
    itk_option define -fncallback fncallback Fncallback ""
    itk_option define -bindentercb bindentercb Bindentercb ""
    itk_option define -bindleavecb bindleavecb Bindleavecb ""
    itk_option define -writetextcb writetextcb Writetextcb ""
    itk_option define -px2dist px2dist Px2dist ""
    itk_option define -units units Units "m"
    itk_option define -bindings bindings Bindings "enable"
    itk_option define -ondelete ondelete Ondelete ""
    itk_option define -onframe onframe Onframe ""


    constructor { name win args } {
        # defined below
    }
    destructor {
        # defined below
    }

    public method Show {args}
    public method Hide {args}
    public method Coords {args}
    public method Frame {args}
    public method Move {status x y}
    public method Menu {args}


    public variable  fncallback ""      ;# framenumber callback - tells what frame we are on
    public variable  bindentercb ""     ;# enter binding callback - call this when entering the object
    public variable  bindleavecb ""     ;# leave binding callback - call this when leaving the object
    public variable  writetextcb ""     ;# write text callback - call this to write text to the canvas

    protected method Enter {}
    protected method Leave {}
    protected method CatchEvent {event}

    protected method _fixValue {args}
    protected method _fixPx2Dist {px2dist}
    protected method _fixBindings {status}

    private variable _canvas        ""  ;# canvas which owns the object
    private variable _name          ""  ;# id of the object
    private variable _color         ""  ;# color of the object
    private variable _frame         0   ;# frame number where the object lives
    private variable _coords        ""  ;# coords of the object, x0 y0 x1 y1
    private variable _x             0   ;# x coord when "pressed" for motion
    private variable _y             0   ;# y coord when "pressed" for motion
    private variable _px2dist       ""  ;# variable associated with -px2dist
    private variable _units         ""  ;#
    private variable _dist          0   ;# distance of the measured space
}

itk::usual VideoDistance {
    keep -background -foreground -cursor -font
    keep -plotbackground -plotforeground
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::VideoDistance::constructor {name win args} {

    set _name $name
    set _canvas $win

    # setup the control menu
    set menu $itk_component(hull).distancecontrols
    itk_component add menu {
        Rappture::Balloon $itk_interior.menu -title "Controls"
    }
    set controls [$itk_component(menu) component inner]

    set fg [option get $itk_component(hull) font Font]
    label $controls.propertiesl -text "Properties" -font $fg \
        -highlightthickness 0

    # Measurement control
    label $controls.measurementl -text "Value" -font $fg \
        -highlightthickness 0
    entry $controls.measuremente -width 5 -background white

    # Frame number control
    label $controls.framenuml -text "Frame" -font "Arial 9"\
         -highlightthickness 0
    Rappture::Spinint $controls.framenume \
        -min 0 -width 5 -font "arial 9"

    # x0
    label $controls.x0l -text "x0" -font $fg -highlightthickness 0
    #FIXME: if the canvas width increases after the distance widget is created,
    #       this max is not updated.
    Rappture::Spinint $controls.x0e \
            -min 0 -max [winfo width ${_canvas}] -width 4 -font "arial 9"

    # y0
    label $controls.y0l -text "y0" -font $fg -highlightthickness 0
    #FIXME: if the canvas height increases after the distance widget is created,
    #       this max is not updated.
    Rappture::Spinint $controls.y0e \
            -min 0 -max [winfo height ${_canvas}] -width 4 -font "arial 9"

    # x1
    label $controls.x1l -text "x1" -font $fg -highlightthickness 0
    #FIXME: if the canvas width increases after the distance widget is created,
    #       this max is not updated.
    Rappture::Spinint $controls.x1e \
            -min 0 -max [winfo width ${_canvas}] -width 4 -font "arial 9"

    # y1
    label $controls.y1l -text "y1" -font $fg -highlightthickness 0
    #FIXME: if the canvas height increases after the distance widget is created,
    #       this max is not updated.
    Rappture::Spinint $controls.y1e \
            -min 0 -max [winfo height ${_canvas}] -width 4 -font "arial 9"

    # Delete control
    label $controls.deletel -text "Delete" -font $fg \
        -highlightthickness 0
    Rappture::Switch $controls.deleteb -showtext "false"
    $controls.deleteb value false

    button $controls.saveb -text Save \
        -relief raised -pady 0 -padx 0  -font "Arial 9" \
        -command [itcl::code $this Menu deactivate save] \
        -activebackground grey90

    button $controls.cancelb -text Cancel \
        -relief raised -pady 0 -padx 0  -font "Arial 9" \
        -command [itcl::code $this Menu deactivate cancel] \
        -activebackground grey90

    grid $controls.measurementl    -column 0 -row 0 -sticky e
    grid $controls.measuremente    -column 1 -row 0 -sticky w
    grid $controls.framenuml       -column 2 -row 0 -sticky e
    grid $controls.framenume       -column 3 -row 0 -sticky w
    grid $controls.x0l             -column 0 -row 1 -sticky e
    grid $controls.x0e             -column 1 -row 1 -sticky w
    grid $controls.y0l             -column 2 -row 1 -sticky e
    grid $controls.y0e             -column 3 -row 1 -sticky w
    grid $controls.x1l             -column 0 -row 2 -sticky e
    grid $controls.x1e             -column 1 -row 2 -sticky w
    grid $controls.y1l             -column 2 -row 2 -sticky e
    grid $controls.y1e             -column 3 -row 2 -sticky w
    grid $controls.deletel         -column 2 -row 3 -sticky e
    grid $controls.deleteb         -column 3 -row 3 -sticky w
    grid $controls.saveb           -column 0 -row 4 -sticky e -columnspan 2
    grid $controls.cancelb         -column 2 -row 4 -sticky w -columnspan 2


    # finish configuring the object
    eval itk_initialize $args

    # set the frame for the particle
    Frame [uplevel \#0 $fncallback]
    bind ${_name}-FrameEvent <<Frame>> [itcl::code $this CatchEvent Frame]
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::VideoDistance::destructor {} {
    configure -px2dist ""  ;# remove variable trace

    Hide object
    _fixBindings disable

    if {"" != $itk_option(-ondelete)} {
        uplevel \#0 $itk_option(-ondelete)
    }

}

# ----------------------------------------------------------------------
#   Frame ?<frameNum>? - update the frame this object is in
# ----------------------------------------------------------------------
itcl::body Rappture::VideoDistance::Frame {args} {
    if {[llength $args] == 1} {
        set val [lindex $args 0]
        if {([string is integer $val] != 1)} {
            error "bad value: \"$val\": frame number should be an integer"
        }

        set _frame $val

        if {"" != $itk_option(-onframe)} {
            uplevel \#0 $itk_option(-onframe) ${_frame}
        }
    } elseif {[llength $args] != 0} {
        error "wrong # args: should be \"Frame ?<frameNumber>?\""
    }
    return ${_frame}
}

# ----------------------------------------------------------------------
#   Coords ?<x0> <y0> <x1> <y1>? - update the coordinates of this object
# ----------------------------------------------------------------------
itcl::body Rappture::VideoDistance::Coords {args} {
    if {[llength $args] == 0} {
        return ${_coords}
    } elseif {[llength $args] == 1} {
        foreach {x0 y0 x1 y1} [lindex $args 0] break
    } elseif {[llength $args] == 4} {
        foreach {x0 y0 x1 y1} $args break
    } else {
        error "wrong # args: should be \"Coords ?<x0> <y0> <x1> <y1>?\""
    }

    if {([string is double $x0] != 1)} {
        error "bad value: \"$x0\": x coordinate should be a double"
    }
    if {([string is double $y0] != 1)} {
        error "bad value: \"$y0\": y coordinate should be a double"
    }
    if {([string is double $x1] != 1)} {
        error "bad value: \"$x1\": x coordinate should be a double"
    }
    if {([string is double $y1] != 1)} {
        error "bad value: \"$y1\": y coordinate should be a double"
    }

    set _coords [list $x0 $y0 $x1 $y1]

    if {[llength [${_canvas} find withtag ${_name}-line]] > 0} {
        eval ${_canvas} coords ${_name}-line ${_coords}
    }

    _fixValue
    return ${_coords}
}

# ----------------------------------------------------------------------
#   Enter - bindings if the mouse enters the object's space
# ----------------------------------------------------------------------
itcl::body Rappture::VideoDistance::Enter {} {
    uplevel \#0 $bindentercb
}

# ----------------------------------------------------------------------
#   Leave - bindings if the mouse leaves the object's space
# ----------------------------------------------------------------------
itcl::body Rappture::VideoDistance::Leave {} {
    uplevel \#0 $bindleavecb
}


# ----------------------------------------------------------------------
#   CatchEvent - bindings for caught events
# ----------------------------------------------------------------------
itcl::body Rappture::VideoDistance::CatchEvent {event} {
    switch -- $event {
        "Frame" {
            if {[uplevel \#0 $fncallback] == ${_frame}} {
                ${_canvas} itemconfigure ${_name}-line -fill red
            } else {
                ${_canvas} itemconfigure ${_name}-line -fill ${_color}
            }
        }
        default {
            error "bad event \"$event\": should be one of Frame."
        }

    }
}


# ----------------------------------------------------------------------
# Show - put properties of the object on the canvas
#   object - draw the object on the canvas
#   name - popup a ballon with the name of this object
# ----------------------------------------------------------------------
itcl::body Rappture::VideoDistance::Show {args} {
    set option [lindex $args 0]
    switch -- $option {
        "object" {
            if {[llength $args] != 1} {
                error "wrong # args: should be \"object\""
            }
            ${_canvas} create line ${_coords} \
                -fill ${_color}\
                -width 2  \
                -tags "measure ${_name} ${_name}-line" \
                -dash {4 4} \
                -arrow both
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
#   object - remove the particle from where it is drawn
#   name - remove the popup with the name
# ----------------------------------------------------------------------
itcl::body Rappture::VideoDistance::Hide {args} {
    set option [lindex $args 0]
    switch -- $option {
        "object" {
            if {[llength $args] != 1} {
                error "wrong # args: should be \"object\""
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
itcl::body Rappture::VideoDistance::Move {status x y} {
    switch -- $status {
        "press" {
            set _x $x
            set _y $y
        }
        "motion" {
            ${_canvas} move ${_name} [expr $x-${_x}] [expr $y-${_y}]
            set _coords [${_canvas} coords ${_name}-line]
            set _x $x
            set _y $y
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
itcl::body Rappture::VideoDistance::Menu {args} {
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
            foreach {x0 y0 x1 y1} ${_coords} break
            $controls.measuremente delete 0 end
            $controls.measuremente insert 0 "${_dist} ${_units}"
            $controls.framenume value ${_frame}
            $controls.x0e value $x0
            $controls.y0e value $y0
            $controls.x1e value $x1
            $controls.y1e value $y1
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

                    foreach {oldx0 oldy0 oldx1 oldy1} ${_coords} break
                    set newx0 [$controls.x0e value]
                    set newy0 [$controls.y0e value]
                    set newx1 [$controls.x1e value]
                    set newy1 [$controls.y1e value]

                    if {$oldx0 != $newx0 ||
                        $oldy0 != $newy0 ||
                        $oldx1 != $newx1 ||
                        $oldy1 != $newy1} {

                        Coords $newx0 $newy0 $newx1 $newy1
                    }

                    set newdist [Rappture::Units::convert \
                        [$controls.measuremente get] \
                        -context ${_units} -units off]

                    if {$newdist != ${_dist}} {
                        # update the distance displayed

                        set px [expr sqrt(pow(($newx1-$newx0),2)+pow(($newy1-$newy0),2))]
                        set px2dist [expr $newdist/$px]

                        _fixPx2Dist $px2dist
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
# _fixBindings - enable/disable bindings
#   enable
#   disable
# ----------------------------------------------------------------------
itcl::body Rappture::VideoDistance::_fixBindings {status} {
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
            bindtags ${_canvas} [concat "${_name}-FrameEvent" [bindtags ${_canvas}]]
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
# USAGE: _fixPx2Dist
# Invoked whenever the value for this object is changed by the user
# via the popup menu.
# ----------------------------------------------------------------------
itcl::body Rappture::VideoDistance::_fixPx2Dist {px2dist} {
    if {"" == $itk_option(-px2dist)} {
        return
    }
    upvar #0 $itk_option(-px2dist) var
    set var $px2dist
}


# ----------------------------------------------------------------------
# USAGE: _fixValue
# Invoked automatically whenever the -px2dist associated with this
# widget is modified.  Copies the value to the current settings for
# the widget.
# ----------------------------------------------------------------------
itcl::body Rappture::VideoDistance::_fixValue {args} {
    if {"" == $itk_option(-px2dist)} {
        return
    }
    upvar #0 $itk_option(-px2dist) var

    if {"" == ${_coords}} {
        # no coords, skip calculation
        return
    }

    # calculate the length
    foreach {x0 y0 x1 y1} ${_coords} break
    set px [expr sqrt(pow(($x1-$x0),2)+pow(($y1-$y0),2))]
    set _dist [expr $px*$var]

    # run the new value through units conversion to round
    # it off so when we show it in the menu and compare it
    # to the value that comes back from the menu, we don't
    # get differences in value due to rounding.
    set _dist [Rappture::Units::convert ${_dist} -context ${_units} -units off]

    set x [expr "$x0 + (($x1-$x0)/2)"]
    set y [expr "$y0 + (($y1-$y0)/2)"]

    set tt "${_dist} ${_units}"
    set tags "meastext ${_name} ${_name}-val"
    set width [expr sqrt(pow(abs($x1-$x0),2)+pow(abs($y1-$y0),2))]
    set args [list $x $y "$tt" "${_color}" "$tags" $width]

    # remove old text
    ${_canvas} delete ${_name}-val

    set controls [$itk_component(menu) component inner]
    if {![$controls.deleteb value]} {
        # if the object is not hidden, write _dist to the canvas
        uplevel \#0 $writetextcb $args
    }
}


# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -color
# ----------------------------------------------------------------------
itcl::configbody Rappture::VideoDistance::color {
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
itcl::configbody Rappture::VideoDistance::px2dist {
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
itcl::configbody Rappture::VideoDistance::units {
    set _units $itk_option(-units)
    # _fixValue
}


# ----------------------------------------------------------------------
# CONFIGURE: -bindings
# ----------------------------------------------------------------------
itcl::configbody Rappture::VideoDistance::bindings {
    _fixBindings $itk_option(-bindings)
}
# ----------------------------------------------------------------------
