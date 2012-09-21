# ----------------------------------------------------------------------
#  EDITOR: color attribute values
#
#  Used within the Instant Rappture builder to edit color values.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
itcl::class AttrColor {
    constructor {win args} {
        Rappture::getopts args params {
            value -tooltip ""
        }
        set _icon [image create photo -width 20 -height 20]

        label $win.sample -image $_icon
        pack $win.sample -side left
        Rappture::Tooltip::for $win.sample $params(-tooltip)

        button $win.change -text "Choose..." -command [itcl::code $this choose]
        pack $win.change -side left
        _redraw ""

        set _win $win
    }
    destructor {
        image delete $_icon
    }

    public method load {val} {
        _redraw $val
    }

    public method check {} {
        if {"" != $_color && [catch {winfo rgb . $_color} err]} {
            return [list error "Bad value \"$_color\": should be color name or #RRGGBB"]
        }
    }

    public method save {var} {
        upvar $var value

        set err [lindex [check] 1]
        if {[string length $err] > 0} {
            Rappture::Tooltip::cue $_win $err
            return 0
        }

        set value $_color
        return 1
    }

    public method edit {} {
        focus -force $_win.change
    }

    public proc import {xmlobj path} {
        # trivial import -- just return info as-is from XML
        return [$xmlobj get $path]
    }

    public proc export {xmlobj path value} {
        # trivial export -- just save info as-is into XML
        if {[string length $value] > 0} {
            $xmlobj put $path $value
        }
    }

    public method choose {} {
        if {"" != $_color} {
            _redraw [tk_chooseColor -title "Rappture: Choose Color" -initialcolor $_color]
        } else {
            _redraw [tk_chooseColor -title "Rappture: Choose Color"]
        }
    }

    private method _redraw {color} {
        set _color $color
        set w [image width $_icon]
        set h [image width $_icon]
        $_icon put white -to 0 0 $w $h
        $_icon put black -to 0 0 $w 1
        $_icon put black -to 0 0 1 $h
        $_icon put black -to [expr {$w-1}] 0 $w $h
        $_icon put black -to 0 [expr {$h-1}] $w $h

        if {"" != $color} {
            $_icon put $color -to 4 4 [expr {$w-4}] [expr {$h-4}]
        } else {
            $_icon copy [Rappture::icon diag] -to 4 4 [expr {$w-4}] [expr {$h-4}]
        }
    }

    private variable _win ""       ;# containing frame
    private variable _icon ""      ;# image for color sample
    private variable _color ""     ;# name of color sample
}
