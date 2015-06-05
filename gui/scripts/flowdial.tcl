# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: Flowdial - selector, like the dial on a flow
#
#  This widget looks like the dial on an old-fashioned car flow.
#  It draws a series of values along an axis, and allows a selector
#  to move back and forth to select the values.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

option add *Flowdial.dialProgressColor #6666cc widgetDefault
option add *Flowdial.thickness 10 widgetDefault
option add *Flowdial.length 2i widgetDefault
option add *Flowdial.knobImage knob widgetDefault
option add *Flowdial.knobPosition n@middle widgetDefault
option add *Flowdial.dialOutlineColor black widgetDefault
option add *Flowdial.dialFillColor white widgetDefault
option add *Flowdial.lineColor gray widgetDefault
option add *Flowdial.activeLineColor black widgetDefault
option add *Flowdial.padding 0 widgetDefault
option add *Flowdial.valueWidth 10 widgetDefault
option add *Flowdial.valuePadding 0.1 widgetDefault
option add *Flowdial.foreground black widgetDefault
option add *Flowdial.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault

itcl::class Rappture::Flowdial {
    inherit itk::Widget

    itk_option define -min min Min ""
    itk_option define -max max Max ""
    itk_option define -variable variable Variable ""

    itk_option define -thickness thickness Thickness 0
    itk_option define -length length Length 0
    itk_option define -padding padding Padding 0

    itk_option define -foreground foreground Foreground "black"
    itk_option define -dialoutlinecolor dialOutlineColor Color "black"
    itk_option define -dialfillcolor dialFillColor Color "white"
    itk_option define -dialprogresscolor dialProgressColor Color ""
    itk_option define -linecolor lineColor Color "black"
    itk_option define -activelinecolor activeLineColor Color "black"
    itk_option define -knobimage knobImage KnobImage ""
    itk_option define -knobposition knobPosition KnobPosition ""

    itk_option define -font font Font ""
    itk_option define -valuewidth valueWidth ValueWidth 0
    itk_option define -valuepadding valuePadding ValuePadding 0


    constructor {args} { # defined below }
    destructor { # defined below }

    public method current {value}
    public method clear {}
    public method color {value}

    protected method _redraw {}
    protected method _click {x y}
    protected method _navigate {offset}
    protected method _fixSize {}
    protected method _fixValue {args}

    private method _current {value}
    private method ms2rel {value}
    private method rel2ms {value}
    private variable _values ""       ;# list of all values on the dial
    private variable _val2label       ;# maps value => string label(s)
    private variable _current ""      ;# current value (where pointer is)
    private variable _variable ""     ;# variable associated with -variable
    private variable _knob ""         ;# image for knob
    private variable _spectrum ""     ;# width allocated for values
    private variable _activecolor ""  ;# width allocated for values
    private variable _vwidth 0        ;# width allocated for values
    public variable min 0.0
    public variable max 1.0
}

itk::usual Flowdial {
    keep -background -foreground -cursor -font
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Flowdial::constructor {args} {
    itk_component add dial {
        canvas $itk_interior.dial
    }
    pack $itk_component(dial) -expand yes -fill both
    bind $itk_component(dial) <Configure> [itcl::code $this _redraw]

    if 0 {
    bind $itk_component(dial) <ButtonPress-1> [itcl::code $this _click %x %y]
    bind $itk_component(dial) <B1-Motion> [itcl::code $this _click %x %y]
    bind $itk_component(dial) <ButtonRelease-1> [itcl::code $this _click %x %y]
    bind $itk_component(hull) <KeyPress-Left> [itcl::code $this _navigate -1]
    bind $itk_component(hull) <KeyPress-Right> [itcl::code $this _navigate 1]
    $itk_component(dial) bind  "knob" <Enter> \
        [list $itk_component(dial) configure -cursor sb_h_double_arrow]
    $itk_component(dial) bind  "knob" <Leave> \
        [list $itk_component(dial) configure -cursor ""]
    }
    eval itk_initialize $args

    _fixSize
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Flowdial::destructor {} {
    configure -variable ""  ;# remove variable trace
    after cancel [itcl::code $this _redraw]
}

# ----------------------------------------------------------------------
# USAGE: current ?<value>?
#
# Clients use this to set a new value for the dial.  Values are always
# sorted in order along the dial.  If the value is not specified,
# then it is created automatically based on the number of elements
# on the dial.
# ----------------------------------------------------------------------
itcl::body Rappture::Flowdial::current {value} {
    if {"" == $value} {
        return
    }
    _current [ms2rel $value]
    event generate $itk_component(hull) <<Value>>
}

# ----------------------------------------------------------------------
# USAGE: _current ?<value>?
#
# Clients use this to set a new value for the dial.  Values are always
# sorted in order along the dial.  If the value is not specified,
# then it is created automatically based on the number of elements
# on the dial.
# ----------------------------------------------------------------------
itcl::body Rappture::Flowdial::_current {relval} {
    if { $relval < 0.0 } {
        set relval 0.0
    }
    if { $relval > 1.0 } {
        set relval 1.0
    }
    set _current $relval
    after cancel [itcl::code $this _redraw]
    after idle [itcl::code $this _redraw]
    if { $_variable != "" } {
        upvar #0 $_variable var
        set var [rel2ms $_current]
    }
}

# ----------------------------------------------------------------------
# USAGE: color <value>
#
# Clients use this to query the color associated with a <value>
# along the dial.
# ----------------------------------------------------------------------
itcl::body Rappture::Flowdial::color {value} {
    if {"" != $_spectrum} {
        set frac [expr {double($value-$min)/($max-$min)}]
        set color [$_spectrum get $frac]
    } else {
        if {$value == $_current} {
            set color $_activecolor
        } else {
            set color $itk_option(-linecolor)
        }
    }
    return $color
}

# ----------------------------------------------------------------------
# USAGE: _redraw
#
# Called automatically whenever the widget changes size to redraw
# all elements within it.
# ----------------------------------------------------------------------
itcl::body Rappture::Flowdial::_redraw {} {
    set c $itk_component(dial)
    $c delete all

    set fg $itk_option(-foreground)

    set w [winfo width $c]
    set h [winfo height $c]
    set p [winfo pixels $c $itk_option(-padding)]
    set t [expr {$itk_option(-thickness)+1}]
    set y1 [expr {$h-1}]

    if {"" != $_knob} {
        set kw [image width $_knob]
        set kh [image height $_knob]

        switch -- $itk_option(-knobposition) {
            n@top - nw@top - ne@top {
                set extra [expr {$t-$kh}]
                if {$extra < 0} {set extra 0}
                set y1 [expr {$h-$extra-1}]
            }
            n@middle - nw@middle - ne@middle {
                set extra [expr {int(ceil($kh-0.5*$t))}]
                if {$extra < 0} {set extra 0}
                set y1 [expr {$h-$extra-1}]
            }
            n@bottom - nw@bottom - ne@bottom {
                set y1 [expr {$h-$kh-1}]
            }

            e@top - w@top - center@top -
            e@bottom - w@bottom - center@bottom {
                set extra [expr {int(ceil(0.5*$kh))}]
                set y1 [expr {$h-$extra-1}]
            }
            e@middle - w@middle - center@middle {
                set extra [expr {int(ceil(0.5*($kh-$t)))}]
                if {$extra < 0} {set extra 0}
                set y1 [expr {$h-$extra-1}]
            }

            s@top - sw@top - se@top -
            s@middle - sw@middle - se@middle -
            s@bottom - sw@bottom - se@bottom {
                set y1 [expr {$h-2}]
            }
        }
    }
    set y0 [expr {$y1-$t}]
    set x0 [expr {$p+1}]
    set x1 [expr {$w-$_vwidth-$p-4}]

    # draw the background rectangle
    $c create rectangle $x0 $y0 $x1 $y1 \
        -outline $itk_option(-dialoutlinecolor) \
        -fill $itk_option(-dialfillcolor)

    # draw the optional progress bar, from start to current
    if {"" != $itk_option(-dialprogresscolor) } {
        set xx1 [expr {$_current*($x1-$x0) + $x0}]
        $c create rectangle [expr {$x0+1}] [expr {$y0+3}] $xx1 [expr {$y1-2}] \
            -outline "" -fill $itk_option(-dialprogresscolor)
    }

    # draw lines for all values
    set x [expr {$_current*($x1-$x0) + $x0}]
    if {"" != $_spectrum} {
        set color [$_spectrum get $_current]
    } else {
        set color $_activecolor
        set thick 3
        if {"" != $color} {
            $c create line $x [expr {$y0+1}] $x $y1 -fill $color -width $thick
        }
    }
    regexp {([nsew]+|center)@} $itk_option(-knobposition) match anchor
    switch -glob -- $itk_option(-knobposition) {
        *@top    { set kpos $y0 }
        *@middle { set kpos [expr {int(ceil(0.5*($y1+$y0)))}] }
        *@bottom { set kpos $y1 }
    }
    $c create image $x $kpos -anchor $anchor -image $_knob -tags "knob"

    # if the -valuewidth is > 0, then make room for the value
    set vw $itk_option(-valuewidth)
    if {$vw > 0 && "" != $_current} {
        set str [lindex $_val2label($_current) 0]
        if {[string length $str] >= $vw} {
            set str "[string range $str 0 [expr {$vw-3}]]..."
        }

        set dy [expr {([font metrics $itk_option(-font) -linespace]
                        - [font metrics $itk_option(-font) -ascent])/2}]

        set id [$c create text [expr {$x1+4}] [expr {($y1+$y0)/2+$dy}] \
            -anchor w -text $str -font $itk_option(-font) -foreground $fg]
        foreach {x0 y0 x1 y1} [$c bbox $id] break
        set x0 [expr {$x0 + 10}]

        # set up a tooltip so you can mouse over truncated values
        Rappture::Tooltip::text $c [lindex $_val2label($_current) 0]
        $c bind $id <Enter> \
            [list ::Rappture::Tooltip::tooltip pending %W +$x0,$y1]
        $c bind $id <Leave> \
            [list ::Rappture::Tooltip::tooltip cancel]
        $c bind $id <ButtonPress> \
            [list ::Rappture::Tooltip::tooltip cancel]
        $c bind $id <KeyPress> \
            [list ::Rappture::Tooltip::tooltip cancel]
    }
}

# ----------------------------------------------------------------------
# USAGE: _click <x> <y>
#
# Called automatically whenever the user clicks or drags on the widget
# to select a value.  Moves the current value to the one nearest the
# click point.  If the value actually changes, it generates a <<Value>>
# event to notify clients.
# ----------------------------------------------------------------------
itcl::body Rappture::Flowdial::_click {x y} {
    set c $itk_component(dial)
    set w [winfo width $c]
    set h [winfo height $c]
    set x0 1
    set x1 [expr {$w-$_vwidth-4}]
    focus $itk_component(hull)
    if {$x >= $x0 && $x <= $x1} {
        current [rel2ms [expr double($x - $x0) / double($x1 - $x0)]]
    }
}

# ----------------------------------------------------------------------
# USAGE: _navigate <offset>
#
# Called automatically whenever the user presses left/right keys
# to nudge the current value left or right by some <offset>.  If the
# value actually changes, it generates a <<Value>> event to notify
# clients.
# ----------------------------------------------------------------------
itcl::body Rappture::Flowdial::_navigate {offset} {
    set index [lsearch -exact $_values $_current]
    if {$index >= 0} {
        incr index $offset
        if {$index >= [llength $_values]} {
            set index [expr {[llength $_values]-1}]
        } elseif {$index < 0} {
            set index 0
        }

        set newval [lindex $_values $index]
        if {$newval != $_current} {
            current $newval
            _redraw

            event generate $itk_component(hull) <<Value>>
        }
    }
}


# ----------------------------------------------------------------------
# USAGE: _fixSize
#
# Used internally to compute the overall size of the widget based
# on the -thickness and -length options.
# ----------------------------------------------------------------------
itcl::body Rappture::Flowdial::_fixSize {} {
    set h [winfo pixels $itk_component(hull) $itk_option(-thickness)]

    if {"" != $_knob} {
        set kh [image height $_knob]

        switch -- $itk_option(-knobposition) {
            n@top - nw@top - ne@top -
            s@bottom - sw@bottom - se@bottom {
                if {$kh > $h} { set h $kh }
            }
            n@middle - nw@middle - ne@middle -
            s@middle - sw@middle - se@middle {
                set h [expr {int(ceil(0.5*$h + $kh))}]
            }
            n@bottom - nw@bottom - ne@bottom -
            s@top - sw@top - se@top {
                set h [expr {$h + $kh}]
            }
            e@middle - w@middle - center@middle {
                set h [expr {(($h > $kh) ? $h : $kh) + 1}]
            }
            n@middle - ne@middle - nw@middle -
            s@middle - se@middle - sw@middle {
                set extra [expr {int(ceil($kh-0.5*$h))}]
                if {$extra < 0} { set extra 0 }
                set h [expr {$h+$extra}]
            }
        }
    }
    incr h 1

    set w [winfo pixels $itk_component(hull) $itk_option(-length)]

    # if the -valuewidth is > 0, then make room for the value
    if {$itk_option(-valuewidth) > 0} {
        set charw [font measure $itk_option(-font) "n"]
        set _vwidth [expr {$itk_option(-valuewidth)*$charw}]
        set w [expr {$w+$_vwidth+4}]
    } else {
        set _vwidth 0
    }

    $itk_component(dial) configure -width $w -height $h
}

# ----------------------------------------------------------------------
# USAGE: _fixValue ?<name1> <name2> <op>?
#
# Invoked automatically whenever the -variable associated with this
# widget is modified.  Copies the value to the current settings for
# the widget.
# ----------------------------------------------------------------------
itcl::body Rappture::Flowdial::_fixValue {args} {
    if {"" == $itk_option(-variable)} {
        return
    }
    upvar #0 $itk_option(-variable) var
    _current [ms2rel $var]
}

itcl::body Rappture::Flowdial::ms2rel { value } {
    if { $max > $min } {
        return [expr {($value - $min) / ($max - $min)}]
    }
    return 0
}

itcl::body Rappture::Flowdial::rel2ms { value } {
    return [expr $value * ($max - $min) + $min]
}

# ----------------------------------------------------------------------
# CONFIGURE: -thickness
# ----------------------------------------------------------------------
itcl::configbody Rappture::Flowdial::thickness {
    _fixSize
}

# ----------------------------------------------------------------------
# CONFIGURE: -length
# ----------------------------------------------------------------------
itcl::configbody Rappture::Flowdial::length {
    _fixSize
}

# ----------------------------------------------------------------------
# CONFIGURE: -font
# ----------------------------------------------------------------------
itcl::configbody Rappture::Flowdial::font {
    _fixSize
}

# ----------------------------------------------------------------------
# CONFIGURE: -valuewidth
# ----------------------------------------------------------------------
itcl::configbody Rappture::Flowdial::valuewidth {
    if {![string is integer $itk_option(-valuewidth)]} {
        error "bad value \"$itk_option(-valuewidth)\": should be integer"
    }
    _fixSize
    after cancel [itcl::code $this _redraw]
    after idle [itcl::code $this _redraw]
}

# ----------------------------------------------------------------------
# CONFIGURE: -foreground
# ----------------------------------------------------------------------
itcl::configbody Rappture::Flowdial::foreground {
    after cancel [itcl::code $this _redraw]
    after idle [itcl::code $this _redraw]
}

# ----------------------------------------------------------------------
# CONFIGURE: -dialoutlinecolor
# ----------------------------------------------------------------------
itcl::configbody Rappture::Flowdial::dialoutlinecolor {
    after cancel [itcl::code $this _redraw]
    after idle [itcl::code $this _redraw]
}

# ----------------------------------------------------------------------
# CONFIGURE: -dialfillcolor
# ----------------------------------------------------------------------
itcl::configbody Rappture::Flowdial::dialfillcolor {
    after cancel [itcl::code $this _redraw]
    after idle [itcl::code $this _redraw]
}

# ----------------------------------------------------------------------
# CONFIGURE: -dialprogresscolor
# ----------------------------------------------------------------------
itcl::configbody Rappture::Flowdial::dialprogresscolor {
    after cancel [itcl::code $this _redraw]
    after idle [itcl::code $this _redraw]
}

# ----------------------------------------------------------------------
# CONFIGURE: -linecolor
# ----------------------------------------------------------------------
itcl::configbody Rappture::Flowdial::linecolor {
    after cancel [itcl::code $this _redraw]
    after idle [itcl::code $this _redraw]
}

# ----------------------------------------------------------------------
# CONFIGURE: -activelinecolor
# ----------------------------------------------------------------------
itcl::configbody Rappture::Flowdial::activelinecolor {
    set val $itk_option(-activelinecolor)
    if {[catch {$val isa ::Rappture::Spectrum} valid] == 0 && $valid} {
        set _spectrum $val
        set _activecolor ""
    } elseif {[catch {winfo rgb $itk_component(hull) $val}] == 0} {
        set _spectrum ""
        set _activecolor $val
    } elseif {"" != $val} {
        error "bad value \"$val\": should be Spectrum object or color"
    }
    after cancel [itcl::code $this _redraw]
    after idle [itcl::code $this _redraw]
}

# ----------------------------------------------------------------------
# CONFIGURE: -knobimage
# ----------------------------------------------------------------------
itcl::configbody Rappture::Flowdial::knobimage {
    if {[regexp {^image[0-9]+$} $itk_option(-knobimage)]} {
        set _knob $itk_option(-knobimage)
    } elseif {"" != $itk_option(-knobimage)} {
        set _knob [Rappture::icon $itk_option(-knobimage)]
    } else {
        set _knob ""
    }
    _fixSize

    after cancel [itcl::code $this _redraw]
    after idle [itcl::code $this _redraw]
}

# ----------------------------------------------------------------------
# CONFIGURE: -knobposition
# ----------------------------------------------------------------------
itcl::configbody Rappture::Flowdial::knobposition {
    if {![regexp {^([nsew]+|center)@(top|middle|bottom)$} $itk_option(-knobposition)]} {
        error "bad value \"$itk_option(-knobposition)\": should be anchor@top|middle|bottom"
    }
    _fixSize

    after cancel [itcl::code $this _redraw]
    after idle [itcl::code $this _redraw]
}

# ----------------------------------------------------------------------
# CONFIGURE: -padding
# This adds padding on left/right side of dial background.
# ----------------------------------------------------------------------
itcl::configbody Rappture::Flowdial::padding {
    if {[catch {winfo pixels $itk_component(hull) $itk_option(-padding)}]} {
        error "bad value \"$itk_option(-padding)\": should be size in pixels"
    }
}

# ----------------------------------------------------------------------
# CONFIGURE: -valuepadding
# This shifts min/max limits in by a fraction of the overall size.
# ----------------------------------------------------------------------
itcl::configbody Rappture::Flowdial::valuepadding {
    if {![string is double $itk_option(-valuepadding)]
          || $itk_option(-valuepadding) < 0} {
        error "bad value \"$itk_option(-valuepadding)\": should be >= 0.0"
    }
}

# ----------------------------------------------------------------------
# CONFIGURE: -variable
# ----------------------------------------------------------------------
itcl::configbody Rappture::Flowdial::variable {
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
