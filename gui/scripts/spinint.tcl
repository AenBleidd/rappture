# ----------------------------------------------------------------------
#  COMPONENT: spinint - spinner for integer values
#
#  This widget is a spinner with up/down arrows for managing integer
#  values.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require BLT

option add *Spinint.width 5 widgetDefault
option add *Spinint.textBackground white widgetDefault

blt::bitmap define Spinint-up {
#define up_width 8
#define up_height 4
static unsigned char up_bits[] = {
   0x10, 0x38, 0x7c, 0xfe};
}

blt::bitmap define Spinint-down {
#define arrow_width 8
#define arrow_height 4
static unsigned char arrow_bits[] = {
   0xfe, 0x7c, 0x38, 0x10};
}

itcl::class Rappture::Spinint {
    inherit itk::Widget

    itk_option define -min min Min ""
    itk_option define -max max Max ""
    itk_option define -delta delta Delta 1

    constructor {args} { # defined below }

    public method value {args}
    public method bump {{delta up}}

    protected method _validate {char}
}

itk::usual Spinint {
    keep -cursor -font
    keep -foreground -background
    keep -textforeground -textbackground
    keep -selectbackground -selectforeground -selectborderwidth
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Spinint::constructor {args} {
    itk_component add entry {
        entry $itk_interior.entry
    } {
        usual
        keep -width
        rename -background -textbackground textBackground Background
        rename -foreground -textforeground textForeground Foreground
        rename -highlightbackground -background background Background
    }
    pack $itk_component(entry) -side left -expand yes -fill x

    bind $itk_component(entry) <KeyPress> \
        [itcl::code $this _validate %A]
    bind $itk_component(entry) <KeyPress-Return> \
        "$this value \[$itk_component(entry) get\]"

    itk_component add controls {
        frame $itk_interior.cntls
    }
    pack $itk_component(controls) -side right

    itk_component add up {
        button $itk_component(controls).spinup -bitmap Spinint-up \
            -borderwidth 1 -relief raised -highlightthickness 0 \
            -command [itcl::code $this bump up]
    } {
        usual
        ignore -borderwidth -highlightthickness
    }
    pack $itk_component(up) -side top -expand yes -fill both

    itk_component add down {
        button $itk_component(controls).spindn -bitmap Spinint-down \
            -borderwidth 1 -relief raised -highlightthickness 0 \
            -command [itcl::code $this bump down]
    } {
        usual
        ignore -borderwidth -highlightthickness
    }
    pack $itk_component(down) -side bottom -expand yes -fill both

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# USAGE: value ?<newval>?
#
# Clients use this to query/set the value for this widget.  With
# no args, it returns the current value for the widget.  If the
# <newval> is specified, it sets the value of the widget and
# sends a <<Value>> event.
# ----------------------------------------------------------------------
itcl::body Rappture::Spinint::value {args} {
    if {[llength $args] == 1} {
        set newval [lindex $args 0]

        if {"" != $newval} {
            if {"" != $itk_option(-min) && $newval < $itk_option(-min)} {
                set newval $itk_option(-min)
            }
            if {"" != $itk_option(-max) && $newval > $itk_option(-max)} {
                set newval $itk_option(-max)
            }
        }

        $itk_component(entry) delete 0 end
        $itk_component(entry) insert 0 $newval
        after 10 [list catch [list event generate $itk_component(hull) <<Value>>]]
    } elseif {[llength $args] != 0} {
        error "wrong # args: should be \"value ?newval?\""
    }
    return [$itk_component(entry) get]
}

# ----------------------------------------------------------------------
# USAGE: bump ?<delta>?
#
# Used internally when you click on the up/down arrows.  Clients
# can also use it directly to bump values up/down.  The optional
# <delta> can be an integer value or the keyword "up" or "down".
# ----------------------------------------------------------------------
itcl::body Rappture::Spinint::bump {{delta up}} {
    if {"up" == $delta} {
        set delta $itk_option(-delta)
    } elseif {"down" == $delta} {
        set delta [expr {-$itk_option(-delta)}]
    } elseif {![string is integer $delta]} {
        error "bad delta \"$delta\": should be up, down, or integer"
    }

    set val [$itk_component(entry) get]
    if {$val == ""} {
        set val 0
    }
    value [expr {$val+$delta}]
}

# ----------------------------------------------------------------------
# USAGE: _validate <char>
#
# Validates each character as it is typed into the spinner.
# If the <char> is not a digit, then this procedure beeps and
# prevents the character from being inserted.
# ----------------------------------------------------------------------
itcl::body Rappture::Spinint::_validate {char} {
    if {[string match "\[ -~\]" $char]} {
        if {![string match "\[0-9\]" $char]} {
            bell
            return -code break
        }
    }
}
