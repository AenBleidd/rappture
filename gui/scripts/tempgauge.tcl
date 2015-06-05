# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: tempgauge - gauge for temperature values
#
#  This is a specialize form of the more general gauge, used for
#  displaying temperature values.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

option add *TemperatureGauge.sampleWidth 30 widgetDefault
option add *TemperatureGauge.sampleHeight 20 widgetDefault
option add *TemperatureGauge.borderWidth 2 widgetDefault
option add *TemperatureGauge.relief sunken widgetDefault
option add *TemperatureGauge.textBackground #cccccc widgetDefault
option add *TemperatureGauge.valuePosition "right" widgetDefault
option add *TemperatureGauge.editable yes widgetDefault
option add *TemperatureGauge.state normal widgetDefault

itcl::class Rappture::TemperatureGauge {
    inherit Rappture::Gauge

    constructor {args} { # defined below }

    protected method _redraw {}
    protected method _resize {}

    # create a spectrum to use for all temperature widgets
    private common _spectrum [Rappture::Spectrum [namespace current]::#auto {
        0.0    blue
        300.0  red
        500.0  yellow
    } -units K]
}

itk::usual TemperatureGauge {
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::TemperatureGauge::constructor {args} {
    eval itk_initialize -spectrum $_spectrum -units K $args
}

# ----------------------------------------------------------------------
# USAGE: _redraw
#
# Used internally to redraw the gauge on the internal canvas based
# on the current value and the size of the widget.  For this temperature
# gauge, we draw something that looks like a thermometer.
# ----------------------------------------------------------------------
itcl::body Rappture::TemperatureGauge::_redraw {} {
    set c $itk_component(icon)
    set w [winfo width $c]
    set h [winfo height $c]

    if {"" == [$c find all]} {
        # first time around, create the items
        $c create oval 0 0 1 1 -outline "" -tags bulbfill
        $c create oval 0 0 1 1 -outline black -tags bulboutline
        $c create oval 0 0 1 1 -outline "" -fill "" -stipple gray50 -tags {bulbscreen screen}
        $c create rect 0 0 1 1 -outline black -fill white -tags stickoutline
        $c create rect 0 0 1 1 -outline "" -tags stickfill
        $c create rect 0 0 1 1 -outline "" -fill "" -stipple gray50 -tags {stickscreen screen}
        $c create image 0 0 -anchor w -image "" -tags bimage
    }

    if {"" != $itk_option(-spectrum)} {
        set color [$itk_option(-spectrum) get [value]]
        set frac [$itk_option(-spectrum) get -fraction [value]]
    } else {
        set color ""
        set frac 0
    }

    # update the items based on current values
    set x 1
    set y [expr {0.5*$h}]
    $c coords bimage 0 $y
    if {$itk_option(-image) != ""} {
        set x [expr {$x + [image width $itk_option(-image)] + 2}]
    }

    set avail [expr {$w-$x}]
    if {$avail > 0} {
        #
        # If we have any space left over, draw the thermometer
        # as a mercury bulb on the left and a stick to the right.
        #
        set bsize [expr {0.2*$avail}]
        if {$bsize > 0.5*$h-2} {set bsize [expr {0.5*$h-2}]}
        set ssize [expr {0.5*$bsize}]

        $c coords bulboutline $x [expr {$y-$bsize}] \
            [expr {$x+2*$bsize}] [expr {$y+$bsize}]
        $c coords bulbscreen [expr {$x-1}] [expr {$y-$bsize-1}] \
            [expr {$x+2*$bsize+1}] [expr {$y+$bsize+1}]
        $c coords bulbfill $x [expr {$y-$bsize}] \
            [expr {$x+2*$bsize}] [expr {$y+$bsize}]

        set x0 [expr {$x+2*$bsize+1}]
        set x1 [expr {$w-2}]
        set xr [expr {($x1-$x0)*$frac + $x0}]
        $c coords stickoutline [expr {$x0-2}] [expr {$y-$ssize}] \
            $x1 [expr {$y+$ssize}]
        $c coords stickscreen [expr {$x0-2}] [expr {$y-$ssize}] \
            [expr {$x1+1}] [expr {$y+$ssize+1}]
        $c coords stickfill [expr {$x0-2}] [expr {$y-$ssize+1}] \
            $xr [expr {$y+$ssize}]

        $c itemconfigure bulbfill -fill $color
        $c itemconfigure stickfill -fill $color
    }

    if {$itk_option(-state) == "disabled"} {
        $c itemconfigure screen -fill white
    } else {
        $c itemconfigure screen -fill ""
    }
}

# ----------------------------------------------------------------------
# USAGE: _resize
#
# Used internally to resize the internal canvas based on the -image
# option or the size of the text.
# ----------------------------------------------------------------------
itcl::body Rappture::TemperatureGauge::_resize {} {
    if {$itk_option(-samplewidth) > 0} {
        set w $itk_option(-samplewidth)
    } else {
        set w [winfo reqheight $itk_component(value)]
    }
    if {$itk_option(-image) != ""} {
        set w [expr {$w+[image width $itk_option(-image)]+4}]
    }

    if {$itk_option(-sampleheight) > 0} {
        set h $itk_option(-sampleheight)
    } else {
        if {$itk_option(-image) != ""} {
            set h [expr {[image height $itk_option(-image)]+4}]
        } else {
            set h [winfo reqheight $itk_component(value)]
        }
    }

    $itk_component(icon) configure -width $w -height $h
}
