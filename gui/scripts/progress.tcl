# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: progress - progress meter for long-running apps
#
#  This widget acts as a progress meter for long-running applications.
#  It accepts progress reports as a combination of a percentage and
#  a message, and then displays the percentage on a bar 0-100%.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

option add *Progress.borderWidth 2 widgetDefault
option add *Progress.relief sunken widgetDefault
option add *Progress.length 2i widgetDefault
option add *Progress.barBackground white widgetDefault
option add *Progress.barColor blue widgetDefault
option add *Progress.font -*-helvetica-medium-r-normal-*-12-* widgetDefault

itcl::class Rappture::Progress {
    inherit itk::Widget

    itk_option define -barcolor barColor BarColor ""
    itk_option define -barbackground barBackground BarBackground ""
    itk_option define -length length Length 0

    constructor {args} { # defined below }
    public method settings {args}

    protected method _redraw {}

    private variable _message ""  ;# status message
    private variable _percent 0   ;# status percentage
}

itk::usual Progress {
    keep -cursor -font -foreground -background
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Progress::constructor {args} {
    itk_component add bar {
        canvas $itk_interior.bar -highlightthickness 0
    } {
        usual
        keep -borderwidth -relief
        ignore -highlightthickness -highlightbackground -highlightcolor
    }
    pack $itk_component(bar) -expand yes -fill both
    bind $itk_component(bar) <Configure> [itcl::code $this _redraw]

    itk_component add message {
        label $itk_interior.mesg -anchor w -width 1
    }

    eval itk_initialize $args
    component hull configure -borderwidth 0

    set h [font metrics $itk_option(-font) -linespace]
    $itk_component(bar) configure -height [expr {$h+4}]
}

# ----------------------------------------------------------------------
# USAGE: settings ?-percent <val>? ?-message <string>?
#
# Clients use this to query/set the settings shown in the progress
# meter.  With no args, it returns a list of the form "-percent v
# -message str".  Otherwise, it interprets the args and updates
# the values.
# ----------------------------------------------------------------------
itcl::body Rappture::Progress::settings {args} {
    if {[llength $args] == 0} {
        return [list -percent $_percent -message $_message]
    }

    Rappture::getopts args params {
        value -percent ""
        value -message "__ignore__"
    }

    set changed 0
    if {$params(-percent) != ""} {
        if {![string is double $params(-percent)]} {
            error "bad value \"$params(-percent)\": should be 0-100"
        }
        if {$params(-percent) < 0} {
            set params(-percent) 0
        }
        if {$params(-percent) > 100} {
            set params(-percent) 100
        }
        set _percent $params(-percent)
        set changed 1
    }
    if {$params(-message) != "__ignore__"} {
        set _message $params(-message)
        set changed 1
    }

    if {$changed} {
        _redraw
        update idletasks
    }
}

# ----------------------------------------------------------------------
# USAGE: _redraw
#
# Used internally to redraw the progress meter on the internal canvas.
# ----------------------------------------------------------------------
itcl::body Rappture::Progress::_redraw {} {
    set w [winfo width $itk_component(bar)]
    set h [winfo height $itk_component(bar)]

    if {[string length $_message] > 0} {
        $itk_component(message) configure -text $_message
        pack $itk_component(message) -fill x
    } else {
        pack forget $itk_component(message)
    }

    if {[$itk_component(bar) find all] == ""} {
        $itk_component(bar) create rectangle 0 0 1 1 \
            -outline "" -fill $itk_option(-barbackground) -tags barbg
        $itk_component(bar) create rectangle 0 0 1 1 \
            -outline "" -fill $itk_option(-barcolor) -tags bar
        $itk_component(bar) create text 0 0 \
            -anchor center -text "" -font $itk_option(-font) -tags number
    }

    set msg [format "%3.0f%%" $_percent]
    $itk_component(bar) itemconfigure number -text $msg

    set barw [expr {0.01*$_percent*$w}]
    $itk_component(bar) coords number [expr {$w/2}] [expr {$h/2}]
    $itk_component(bar) coords bar 0 0 $barw $h
    $itk_component(bar) coords barbg 0 0 $w $h
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -barcolor
# ----------------------------------------------------------------------
itcl::configbody Rappture::Progress::barcolor {
    $itk_component(bar) itemconfigure bar -fill $itk_option(-barcolor)
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -barbackground
# ----------------------------------------------------------------------
itcl::configbody Rappture::Progress::barbackground {
    $itk_component(bar) itemconfigure barbg -fill $itk_option(-barbackground)
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -length
# ----------------------------------------------------------------------
itcl::configbody Rappture::Progress::length {
    set w [winfo pixels $itk_component(hull) $itk_option(-length)]
    $itk_component(bar) configure -width $w
}
