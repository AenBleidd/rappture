# ----------------------------------------------------------------------
#  COMPONENT: Radiodial - selector, like the dial on a car radio
#
#  This widget looks like the dial on an old-fashioned car radio.
#  It draws a series of values along an axis, and allows a selector
#  to move back and forth to select the values.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005
#  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Itk

option add *Radiodial.thickness 10 widgetDefault
option add *Radiodial.length 2i widgetDefault
option add *Radiodial.dialOutlineColor black widgetDefault
option add *Radiodial.dialFillColor white widgetDefault
option add *Radiodial.lineColor gray widgetDefault
option add *Radiodial.activeLineColor red widgetDefault
option add *Radiodial.valueWidth 10 widgetDefault
option add *Radiodial.font \
    -*-helvetica-medium-r-normal-*-*-120-* widgetDefault

itcl::class Rappture::Radiodial {
    inherit itk::Widget

    itk_option define -min min Min ""
    itk_option define -max max Max ""
    itk_option define -thickness thickness Thickness 0
    itk_option define -length length Length 0

    itk_option define -dialoutlinecolor dialOutlineColor Color "black"
    itk_option define -dialfillcolor dialFillColor Color "white"
    itk_option define -linecolor lineColor Color "black"
    itk_option define -activelinecolor activeLineColor Color "black"

    itk_option define -font font Font ""
    itk_option define -valuewidth valueWidth ValueWidth 0


    constructor {args} { # defined below }
    destructor { # defined below }

    public method add {label {value ""}}
    public method clear {}
    public method get {args}
    public method current {args}
    public method color {value}
                                                                                
    protected method _redraw {}
    protected method _click {x y}
    protected method _navigate {offset}
    protected method _limits {}
    protected method _fixSize {}

    private variable _values ""       ;# list of all values on the dial
    private variable _val2label       ;# maps value => label
    private variable _current ""      ;# current value (where pointer is)

    private variable _spectrum ""     ;# width allocated for values
    private variable _activecolor ""  ;# width allocated for values
    private variable _vwidth 0        ;# width allocated for values

    #
    # Load the image for the knob.
    #
    private common images
    set images(knob) [image create photo -data {
R0lGODlhCQAMAMIEAAQCBJyanNza3Pz+/P///////////////yH5BAEKAAQALAAAAAAJAAwAAAMj
SEqwDqO9MYJkVASLh/gbAHmgNX6amZXimrbVFkKyLN44kAAAOw==
}]
}
                                                                                
itk::usual Radiodial {
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Radiodial::constructor {args} {
    itk_component add dial {
        canvas $itk_interior.dial
    }
    pack $itk_component(dial) -expand yes -fill both
    bind $itk_component(dial) <Configure> [itcl::code $this _redraw]

    bind $itk_component(dial) <ButtonPress-1> [itcl::code $this _click %x %y]
    bind $itk_component(dial) <B1-Motion> [itcl::code $this _click %x %y]
    bind $itk_component(dial) <ButtonRelease-1> [itcl::code $this _click %x %y]

    bind $itk_component(hull) <KeyPress-Left> [itcl::code $this _navigate -1]
    bind $itk_component(hull) <KeyPress-Right> [itcl::code $this _navigate 1]

    eval itk_initialize $args

    _fixSize
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Radiodial::destructor {} {
    after cancel [itcl::code $this _redraw]
}

# ----------------------------------------------------------------------
# USAGE: add <label> ?<value>?
#
# Clients use this to add new values to the dial.  Values are always
# sorted in order along the dial.  If the value is not specified,
# then it is created automatically based on the number of elements
# on the dial.
# ----------------------------------------------------------------------
itcl::body Rappture::Radiodial::add {label {value ""}} {
    if {"" == $value} {
        set value [llength $_values]
    }
    lappend _values $value
    set _values [lsort -real $_values]
    set _val2label($value) $label

    if {"" == $_current} {
        set _current $value
    }

    after cancel [itcl::code $this _redraw]
    after idle [itcl::code $this _redraw]
}

# ----------------------------------------------------------------------
# USAGE: clear
#
# Clients use this to remove all existing values from the dial.
# ----------------------------------------------------------------------
itcl::body Rappture::Radiodial::clear {} {
    set _values ""
    set _current ""
    catch {unset _val2label}

    after cancel [itcl::code $this _redraw]
    after idle [itcl::code $this _redraw]
}

# ----------------------------------------------------------------------
# USAGE: get ?-format what? ?current|@index?
#
# Clients use this to query values within this radiodial.  With no
# args, it returns a list of all values stored in the widget.  The
# "current" arg requests only the current value on the radiodial.
# The @index syntax can be used to request a particular value at
# an index within the list of values.
#
# By default, this method returns the label for each value.  The
# format option can be used to request the label, the value, or
# both.
# ----------------------------------------------------------------------
itcl::body Rappture::Radiodial::get {args} {
    Rappture::getopts args params {
        value -format "label"
    }
    if {[llength $args] > 1} {
        error "wrong # args: should be \"get ?-format f? ?current|@index\""
    }
    set index [lindex $args 0]
    if {"" == $index} {
        set ilist ""
        for {set i 0} {$i < [llength $_values]} {incr i} {
            append ilist $i
        }
    } elseif {"current" == $index} {
        set ilist [lsearch -exact $_values $_current]
        if {$ilist < 0} {
            set ilist ""
        }
    } elseif {[regexp {^@([0-9]+|end)$} $index match i]} {
        set ilist $i
    }
    if {[llength $ilist] == 1} {
        set op set
    } else {
        set op lappend
    }

    set rlist ""
    foreach i $ilist {
        switch -- $params(-format) {
            label {
                set v [lindex $_values $i]
                $op rlist $_val2label($v)
            }
            value {
                $op rlist [lindex $_values $i]
            }
            all {
                set v [lindex $_values $i]
                $op rlist [list $_val2label($v) $v]
            }
            default {
                error "bad value \"$v\": should be label, value, all"
            }
        }
    }
    return $rlist
}

# ----------------------------------------------------------------------
# USAGE: current ?<newval>?
#
# Clients use this to get/set the current value for this widget.
# ----------------------------------------------------------------------
itcl::body Rappture::Radiodial::current {args} {
    if {[llength $args] == 0} {
        return $_current
    } elseif {[llength $args] == 1} {
        set newval [lindex $args 0]
        set found 0
        foreach v $_values {
            if {[string equal $_val2label($v) $newval]} {
                set newval $v
                set found 1
                break
            }
        }
        if {!$found} {
            error "bad value \"$newval\""
        }
        set _current $newval
        return $_current
    }
    error "wrong # args: should be \"current ?newval?\""
}

# ----------------------------------------------------------------------
# USAGE: color <value>
#
# Clients use this to query the color associated with a <value>
# along the dial.
# ----------------------------------------------------------------------
itcl::body Rappture::Radiodial::color {value} {
    set found 0
    foreach v $_values {
        if {[string equal $_val2label($v) $value]} {
            set value $v
            set found 1
            break
        }
    }
    if {!$found} {
        error "bad value \"$value\""
    }

    if {"" != $_spectrum} {
        foreach {min max} [_limits] break
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
itcl::body Rappture::Radiodial::_redraw {} {
    set c $itk_component(dial)
    $c delete all

    set w [winfo width $c]
    set h [winfo height $c]
    set y1 [expr {$h-[image height $images(knob)]/2-1}]
    set y0 [expr {$y1 - $itk_option(-thickness)-1}]
    set x0 1
    set x1 [expr {$w-$_vwidth-4}]

    # draw the background rectangle
    $c create rectangle $x0 $y0 $x1 $y1 \
        -outline $itk_option(-dialoutlinecolor) \
        -fill $itk_option(-dialfillcolor)

    # draw lines for all values
    foreach {min max} [_limits] break
    if {$max > $min} {
        foreach v $_values {
            set frac [expr {double($v-$min)/($max-$min)}]
            if {"" != $_spectrum} {
                set color [$_spectrum get $frac]
            } else {
                if {$v == $_current} {
                    set color $_activecolor
                } else {
                    set color $itk_option(-linecolor)
                }
            }
            set thick [expr {($v == $_current) ? 3 : 1}]

            set x [expr {$frac*($x1-$x0) + $x0}]
            $c create line $x [expr {$y0+1}] $x $y1 -fill $color -width $thick
        }

        if {"" != $_current} {
            set x [expr {double($_current-$min)/($max-$min)*($x1-$x0) + $x0}]
            $c create image $x [expr {$h-1}] -anchor s -image $images(knob)
        }
    }

    # if the -valuewidth is > 0, then make room for the value
    set vw $itk_option(-valuewidth)
    if {$vw > 0 && "" != $_current} {
        set str $_val2label($_current)
        if {[string length $str] >= $vw} {
            set str "[string range $str 0 [expr {$vw-3}]]..."
        }

        set dy [expr {([font metrics $itk_option(-font) -linespace]
                        - [font metrics $itk_option(-font) -ascent])/2}]

        set id [$c create text [expr {$x1+4}] [expr {($y1+$y0)/2+$dy}] \
            -anchor w -text $str -font $itk_option(-font)]
        foreach {x0 y0 x1 y1} [$c bbox $id] break
        set x0 [expr {$x0 + 10}]

        # set up a tooltip so you can mouse over truncated values
        Rappture::Tooltip::text $c $_val2label($_current)
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
itcl::body Rappture::Radiodial::_click {x y} {
    set c $itk_component(dial)
    set w [winfo width $c]
    set h [winfo height $c]
    set x0 1
    set x1 [expr {$w-$_vwidth-4}]

    focus $itk_component(hull)

    # draw lines for all values
    foreach {min max} [_limits] break
    if {$max > $min && $x >= $x0 && $x <= $x1} {
        set dmin $w
        set xnearest 0
        set vnearest ""
        foreach v $_values {
            set xv [expr {double($v-$min)/($max-$min)*($x1-$x0) + $x0}]
            if {abs($xv-$x) < $dmin} {
                set dmin [expr {abs($xv-$x)}]
                set xnearest $xv
                set vnearest $v
            }
        }

        if {$vnearest != $_current} {
            set _current $vnearest
            _redraw

            event generate $itk_component(hull) <<Value>>
        }
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
itcl::body Rappture::Radiodial::_navigate {offset} {
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
            set _current $newval
            _redraw

            event generate $itk_component(hull) <<Value>>
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _limits
#
# Used internally to compute the overall min/max limits for the
# radio dial.  Returns {min max}, representing the end values for
# the scale.
# ----------------------------------------------------------------------
itcl::body Rappture::Radiodial::_limits {} {
    if {[llength $_values] == 0} {
        set min 0
        set max 0
    } else {
        set min [lindex $_values 0]
        set max $min
        foreach v [lrange $_values 1 end] {
            if {$v < $min} { set min $v }
            if {$v > $max} { set max $v }
        }
        set del [expr {$max-$min}]
        set min [expr {$min-0.1*$del}]
        set max [expr {$max+0.1*$del}]
    }

    if {"" != $itk_option(-min)} {
        set min $itk_option(-min)
    }
    if {"" != $itk_option(-max)} {
        set max $itk_option(-max)
    }
    return [list $min $max]
}

# ----------------------------------------------------------------------
# USAGE: _fixSize
#
# Used internally to compute the overall size of the widget based
# on the -thickness and -length options.
# ----------------------------------------------------------------------
itcl::body Rappture::Radiodial::_fixSize {} {
    set h [winfo pixels $itk_component(hull) $itk_option(-thickness)]
    set h [expr {$h/2 + [image height $images(knob)]}]

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
# CONFIGURE: -thickness
# ----------------------------------------------------------------------
itcl::configbody Rappture::Radiodial::thickness {
    _fixSize
}

# ----------------------------------------------------------------------
# CONFIGURE: -length
# ----------------------------------------------------------------------
itcl::configbody Rappture::Radiodial::length {
    _fixSize
}

# ----------------------------------------------------------------------
# CONFIGURE: -font
# ----------------------------------------------------------------------
itcl::configbody Rappture::Radiodial::font {
    _fixSize
}

# ----------------------------------------------------------------------
# CONFIGURE: -valuewidth
# ----------------------------------------------------------------------
itcl::configbody Rappture::Radiodial::valuewidth {
    if {![string is integer $itk_option(-valuewidth)]} {
        error "bad value \"$itk_option(-valuewidth)\": should be integer"
    }
    after cancel [itcl::code $this _redraw]
    after idle [itcl::code $this _redraw]
}

# ----------------------------------------------------------------------
# CONFIGURE: -dialoutlinecolor
# ----------------------------------------------------------------------
itcl::configbody Rappture::Radiodial::dialoutlinecolor {
    after cancel [itcl::code $this _redraw]
    after idle [itcl::code $this _redraw]
}

# ----------------------------------------------------------------------
# CONFIGURE: -dialfillcolor
# ----------------------------------------------------------------------
itcl::configbody Rappture::Radiodial::dialfillcolor {
    after cancel [itcl::code $this _redraw]
    after idle [itcl::code $this _redraw]
}

# ----------------------------------------------------------------------
# CONFIGURE: -linecolor
# ----------------------------------------------------------------------
itcl::configbody Rappture::Radiodial::linecolor {
    after cancel [itcl::code $this _redraw]
    after idle [itcl::code $this _redraw]
}

# ----------------------------------------------------------------------
# CONFIGURE: -activelinecolor
# ----------------------------------------------------------------------
itcl::configbody Rappture::Radiodial::activelinecolor {
    set val $itk_option(-activelinecolor)
    if {[catch {$val isa ::Rappture::Spectrum} valid] == 0 && $valid} {
        set _spectrum $val
        set _activecolor ""
    } elseif {[catch {winfo rgb $itk_component(hull) $val}] == 0} {
        set _spectrum ""
        set _activecolor $val
    } else {
        error "bad value \"$val\": should be Spectrum object or color"
    }
    after cancel [itcl::code $this _redraw]
    after idle [itcl::code $this _redraw]
}
