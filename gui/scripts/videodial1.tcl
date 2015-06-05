# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: Videodial1 - selector, like the dial on a flow
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
package require BLT

option add *Videodial1.dialProgressColor #6666cc widgetDefault
option add *Videodial1.thickness 10 widgetDefault
option add *Videodial1.length 2i widgetDefault
option add *Videodial1.knobImage knob widgetDefault
option add *Videodial1.knobPosition n@middle widgetDefault
option add *Videodial1.dialOutlineColor black widgetDefault
option add *Videodial1.dialFillColor white widgetDefault
option add *Videodial1.lineColor gray widgetDefault
option add *Videodial1.activeLineColor black widgetDefault
option add *Videodial1.padding 0 widgetDefault
option add *Videodial1.valueWidth 10 widgetDefault
option add *Videodial1.valuePadding 0.1 widgetDefault
option add *Videodial1.foreground black widgetDefault
option add *Videodial1.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault

itcl::class Rappture::Videodial1 {
    inherit itk::Widget

    itk_option define -min min Min 0
    itk_option define -max max Max 1
    itk_option define -variable variable Variable ""
    itk_option define -offset offset Offset 1

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

    protected method _redraw {}
    protected method _knob {x y}
    protected method _navigate {offset}
    protected method _fixSize {}
    protected method _fixValue {args}
    protected method _fixOffsets {}

    private method _current {value}
    private method _draw_major_timeline {}
    private method ms2rel {value}
    private method rel2ms {value}
    private common _click             ;# x,y point where user clicked
    private variable _values ""       ;# list of all values on the dial
    private variable _val2label       ;# maps value => string label(s)
    private variable _current 0       ;# current value (where pointer is)
    private variable _variable ""     ;# variable associated with -variable
    private variable _knob ""         ;# image for knob
    private variable _spectrum ""     ;# width allocated for values
    private variable _activecolor ""  ;# width allocated for values
    private variable _vwidth 0        ;# width allocated for values
    private variable _offset_pos 1    ;#
    private variable _offset_neg -1   ;#
    private variable _imspace 10      ;# pixels between intermediate marks
    private variable _pmcnt 0         ;# particle marker count
    private variable _min 0
    private variable _max 1
}

itk::usual Videodial1 {
    keep -foreground -cursor -font
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial1::constructor {args} {

    # bind $itk_component(hull) <<Frame>> [itcl::code $this _updateCurrent]

    # ----------------------------------------------------------------------
    # controls for the major timeline.
    # ----------------------------------------------------------------------
    itk_component add majordial {
        canvas $itk_interior.majordial
    }

    bind $itk_component(majordial) <Configure> [itcl::code $this _draw_major_timeline]

    bind $itk_component(majordial) <ButtonPress-1> [itcl::code $this _knob %x %y]
    bind $itk_component(majordial) <B1-Motion> [itcl::code $this _knob %x %y]
    bind $itk_component(majordial) <ButtonRelease-1> [itcl::code $this _knob %x %y]

    #bind $itk_component(hull) <KeyPress-Left> [itcl::code $this _navigate $_offset_neg]
    #bind $itk_component(hull) <KeyPress-Right> [itcl::code $this _navigate $_offset_pos]

    $itk_component(majordial) bind  "knob" <Enter> \
        [list $itk_component(majordial) configure -cursor sb_h_double_arrow]
    $itk_component(majordial) bind  "knob" <Leave> \
        [list $itk_component(majordial) configure -cursor ""]

    # ----------------------------------------------------------------------
    # place controls in widget.
    # ----------------------------------------------------------------------

    blt::table $itk_interior \
        0,0 $itk_component(majordial) -fill x

    blt::table configure $itk_interior c* -resize both
    blt::table configure $itk_interior r0 -resize none


    eval itk_initialize $args

    $itk_component(majordial) configure -background green

    _fixSize
    _fixOffsets
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial1::destructor {} {
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
itcl::body Rappture::Videodial1::current {value} {
    if {"" == $value} {
        return
    }
    _current [ms2rel $value]
}

# ----------------------------------------------------------------------
# USAGE: _current ?<value>?
#
# Clients use this to set a new value for the dial.  Values are always
# sorted in order along the dial.  If the value is not specified,
# then it is created automatically based on the number of elements
# on the dial.
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial1::_current {relval} {
    if { $relval < 0.0 } {
        set relval 0.0
    }
    if { $relval > 1.0 } {
        set relval 1.0
    }
    set _current $relval

    after cancel [itcl::code $this _draw_major_timeline]
    after idle [itcl::code $this _draw_major_timeline]

    set framenum [expr round([rel2ms $_current])]

    # update the upvar variable
    if { $_variable != "" } {
        upvar #0 $_variable var
        set var $framenum
    }
}

# ----------------------------------------------------------------------
# USAGE: _draw_major_timeline
#
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial1::_draw_major_timeline {} {
    set c $itk_component(majordial)
    $c delete all

    set fg $itk_option(-foreground)

    set w [winfo width $c]
    set h [winfo height $c]
    set p [winfo pixels $c $itk_option(-padding)]
    set t [expr {$itk_option(-thickness)+1}]
    # FIXME: hack to get the reduce spacing in widget
    set y1 [expr {$h-2}]

    if {"" != $_knob} {
        set kw [image width $_knob]
        set kh [image height $_knob]

        # anchor refers to where on knob
        # top/middle/bottom refers to where on the dial
        # leave room for the bottom of the knob if needed
        switch -- $itk_option(-knobposition) {
            n@top - nw@top - ne@top {
                set extra [expr {$t-$kh}]
                if {$extra < 0} {set extra 0}
                set y1 [expr {$y1-$extra}]
            }
            n@middle - nw@middle - ne@middle {
                set extra [expr {int(ceil($kh-0.5*$t))}]
                if {$extra < 0} {set extra 0}
                set y1 [expr {$y1-$extra}]
            }
            n@bottom - nw@bottom - ne@bottom {
               set y1 [expr {$y1-$kh}]
            }

            e@top - w@top - center@top -
            e@bottom - w@bottom - center@bottom {
                set extra [expr {int(ceil(0.5*$kh))}]
                set y1 [expr {$y1-$extra}]
            }
            e@middle - w@middle - center@middle {
                set extra [expr {int(ceil(0.5*($kh-$t)))}]
                if {$extra < 0} {set extra 0}
                set y1 [expr {$y1-$extra}]
            }

            s@top - sw@top - se@top -
            s@middle - sw@middle - se@middle -
            s@bottom - sw@bottom - se@bottom {
                set y1 [expr {$y1-1}]
            }
        }
    }
    set y0 [expr {$y1-$t}]
    set x0 [expr {$p+1}]
    set x1 [expr {$w-$_vwidth-$p-4}]

    # draw the background rectangle for the major time line
    $c create rectangle $x0 $y0 $x1 $y1 \
        -outline $itk_option(-dialoutlinecolor) \
        -fill $itk_option(-dialfillcolor) \
        -tags "majorbg"

    # draw the optional progress bar for the major time line,
    # from start to current
    if {"" != $itk_option(-dialprogresscolor) } {
        set xx1 [expr {$_current*($x1-$x0) + $x0}]
        $c create rectangle [expr {$x0+1}] [expr {$y0+3}] $xx1 [expr {$y1-2}] \
            -outline "" -fill $itk_option(-dialprogresscolor)
    }

    regexp {([nsew]+|center)@} $itk_option(-knobposition) match anchor
    switch -glob -- $itk_option(-knobposition) {
        *@top    { set kpos $y0 }
        *@middle { set kpos [expr {int(ceil(0.5*($y1+$y0)))}] }
        *@bottom { set kpos $y1 }
    }

    set x [expr {$_current*($x1-$x0) + $x0}]

    set color $_activecolor
    set thick 3
    if {"" != $color} {
        $c create line $x [expr {$y0+1}] $x $y1 -fill $color -width $thick
    }

    $c create image $x $kpos -anchor $anchor -image $_knob -tags "knob"
}

# ----------------------------------------------------------------------
# USAGE: _redraw
#
# Called automatically whenever the widget changes size to redraw
# all elements within it.
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial1::_redraw {} {
    _draw_major_timeline
}

# ----------------------------------------------------------------------
# USAGE: _knob <x> <y>
#
# Called automatically whenever the user clicks or drags on the widget
# to select a value.  Moves the current value to the one nearest the
# click point.  If the value actually changes, it generates a <<Value>>
# event to notify clients.
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial1::_knob {x y} {
    set c $itk_component(majordial)
    set w [winfo width $c]
    set h [winfo height $c]
    set x0 1
    set x1 [expr {$w-$_vwidth-4}]
    focus $itk_component(hull)
    if {$x >= $x0 && $x <= $x1} {
        current [rel2ms [expr double($x - $x0) / double($x1 - $x0)]]
    }
    event generate $itk_component(hull) <<Value>>
}

# ----------------------------------------------------------------------
# USAGE: _navigate <offset>
#
# Called automatically whenever the user presses left/right keys
# to nudge the current value left or right by some <offset>.  If the
# value actually changes, it generates a <<Value>> event to notify
# clients.
# ----------------------------------------------------------------------
#itcl::body Rappture::Videodial1::_navigate {offset} {
#    set index [lsearch -exact $_values $_current]
#    if {$index >= 0} {
#        incr index $offset
#        if {$index >= [llength $_values]} {
#            set index [expr {[llength $_values]-1}]
#        } elseif {$index < 0} {
#            set index 0
#        }
#
#        set newval [lindex $_values $index]
#        if {$newval != $_current} {
#            current $newval
#            _redraw
#
#            event generate $itk_component(hull) <<Value>>
#        }
#    }
#}


# ----------------------------------------------------------------------
# USAGE: _navigate <offset>
#
# Called automatically whenever the user presses left/right keys
# to nudge the current value left or right by some <offset>.  If the
# value actually changes, it generates a <<Value>> event to notify
# clients.
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial1::_navigate {offset} {
    _current [ms2rel [expr [rel2ms ${_current}] + $offset]]
    event generate $itk_component(hull) <<Value>>
}


# ----------------------------------------------------------------------
# USAGE: _fixSize
#
# Used internally to compute the overall size of the widget based
# on the -thickness and -length options.
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial1::_fixSize {} {
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
                set h [expr {(($h > $kh) ? $h : ($kh+1))}]
            }
            n@middle - ne@middle - nw@middle -
            s@middle - se@middle - sw@middle {
                set extra [expr {int(ceil($kh-0.5*$h))}]
                if {$extra < 0} { set extra 0 }
                set h [expr {$h+$extra}]
            }
        }
    }
    # FIXME: hack to get the reduce spacing in widget
    incr h -1

    set w [winfo pixels $itk_component(hull) $itk_option(-length)]

    # if the -valuewidth is > 0, then make room for the value
    if {$itk_option(-valuewidth) > 0} {
        set charw [font measure $itk_option(-font) "n"]
        set _vwidth [expr {$itk_option(-valuewidth)*$charw}]
        set w [expr {$w+$_vwidth+4}]
    } else {
        set _vwidth 0
    }

    $itk_component(majordial) configure -width $w -height $h
}

# ----------------------------------------------------------------------
# USAGE: _fixValue ?<name1> <name2> <op>?
#
# Invoked automatically whenever the -variable associated with this
# widget is modified.  Copies the value to the current settings for
# the widget.
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial1::_fixValue {args} {
    if {"" == $itk_option(-variable)} {
        return
    }
    upvar #0 $itk_option(-variable) var
    _current [ms2rel $var]
}

# ----------------------------------------------------------------------
# USAGE: _fixOffsets
#
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial1::_fixOffsets {} {
    if {0 == $itk_option(-offset)} {
        return
    }
    set _offset_pos $itk_option(-offset)
    set _offset_neg [expr -1*$_offset_pos]
    bind $itk_component(hull) <KeyPress-Left> [itcl::code $this _navigate $_offset_neg]
    bind $itk_component(hull) <KeyPress-Right> [itcl::code $this _navigate $_offset_pos]
}

# ----------------------------------------------------------------------
# USAGE: ms2rel
#
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial1::ms2rel { value } {
    if { ${_max} > ${_min} } {
        return [expr {1.0 * ($value - ${_min}) / (${_max} - ${_min})}]
    }
    return 0
}

# ----------------------------------------------------------------------
# USAGE: rel2ms
#
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial1::rel2ms { value } {
    return [expr $value * (${_max} - ${_min}) + ${_min}]
}

# ----------------------------------------------------------------------
# CONFIGURE: -thickness
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial1::thickness {
    _fixSize
}

# ----------------------------------------------------------------------
# CONFIGURE: -length
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial1::length {
    _fixSize
}

# ----------------------------------------------------------------------
# CONFIGURE: -font
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial1::font {
    _fixSize
}

# ----------------------------------------------------------------------
# CONFIGURE: -valuewidth
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial1::valuewidth {
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
itcl::configbody Rappture::Videodial1::foreground {
    after cancel [itcl::code $this _redraw]
    after idle [itcl::code $this _redraw]
}

# ----------------------------------------------------------------------
# CONFIGURE: -dialoutlinecolor
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial1::dialoutlinecolor {
    after cancel [itcl::code $this _redraw]
    after idle [itcl::code $this _redraw]
}

# ----------------------------------------------------------------------
# CONFIGURE: -dialfillcolor
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial1::dialfillcolor {
    after cancel [itcl::code $this _redraw]
    after idle [itcl::code $this _redraw]
}

# ----------------------------------------------------------------------
# CONFIGURE: -dialprogresscolor
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial1::dialprogresscolor {
    after cancel [itcl::code $this _redraw]
    after idle [itcl::code $this _redraw]
}

# ----------------------------------------------------------------------
# CONFIGURE: -linecolor
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial1::linecolor {
    after cancel [itcl::code $this _redraw]
    after idle [itcl::code $this _redraw]
}

# ----------------------------------------------------------------------
# CONFIGURE: -activelinecolor
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial1::activelinecolor {
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
itcl::configbody Rappture::Videodial1::knobimage {
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
itcl::configbody Rappture::Videodial1::knobposition {
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
itcl::configbody Rappture::Videodial1::padding {
    if {[catch {winfo pixels $itk_component(hull) $itk_option(-padding)}]} {
        error "bad value \"$itk_option(-padding)\": should be size in pixels"
    }
}

# ----------------------------------------------------------------------
# CONFIGURE: -valuepadding
# This shifts min/max limits in by a fraction of the overall size.
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial1::valuepadding {
    if {![string is double $itk_option(-valuepadding)]
          || $itk_option(-valuepadding) < 0} {
        error "bad value \"$itk_option(-valuepadding)\": should be >= 0.0"
    }
}

# ----------------------------------------------------------------------
# CONFIGURE: -variable
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial1::variable {
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
# CONFIGURE: -offset
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial1::offset {
    if {![string is double $itk_option(-offset)]} {
        error "bad value \"$itk_option(-offset)\": should be >= 0.0"
    }
    _fixOffsets
}

# ----------------------------------------------------------------------
# CONFIGURE: -min
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial1::min {
    if {![string is integer $itk_option(-min)]} {
        error "bad value \"$itk_option(-min)\": should be an integer"
    }
    if {$itk_option(-min) < 0} {
        error "bad value \"$itk_option(-min)\": should be >= 0"
    }
    set _min $itk_option(-min)
}

# ----------------------------------------------------------------------
# CONFIGURE: -max
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial1::max {
    if {![string is integer $itk_option(-max)]} {
        error "bad value \"$itk_option(-max)\": should be an integer"
    }
    if {$itk_option(-max) < 0} {
        error "bad value \"$itk_option(-max)\": should be >= 0"
    }
    set _max $itk_option(-max)
}
