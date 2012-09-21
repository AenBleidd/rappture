# ----------------------------------------------------------------------
#  EDITOR: boolean attribute values
#
#  Used within the Instant Rappture builder to edit boolean values.
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
itcl::class AttrBoolean {
    constructor {win args} {
        Rappture::getopts args params {
            value -tooltip ""
        }

        Rappture::Combobox $win.val -width 10 -editable no
        pack $win.val -side left
        $win.val choices insert end 1 "True" 0 "False"

        Rappture::Tooltip::for $win.val $params(-tooltip)

        set _win $win
    }

    public method check {} {
        set val [$_win.val translate [$_win.val value]]
        if {![string is boolean -strict $val]} {
            return [list error "Bad value \"$val\": should be Boolean"]
        }
        return ""
    }

    public method load {val} {
        if {![string is boolean -strict $val] || $val} {
            $_win.val value "True"
        } else {
            $_win.val value "False"
        }
    }

    public method save {var} {
        upvar $var value
        set str [$_win.val translate [$_win.val value]]
        if {[string is boolean -strict $str] && $str} {
            set value "yes"
        } else {
            set value "no"
        }
        return 1
    }

    public method edit {} {
        focus -force $_win.val
    }

    public proc import {xmlobj path} {
        set val [$xmlobj get $path]
        if {[string is boolean -strict $val] && $val} {
            set val "yes"
        } else {
            set val "no"
        }
        return $val
    }

    public proc export {xmlobj path value} {
        if {$value} {
            $xmlobj put $path "yes"
        } else {
            $xmlobj put $path "no"
        }
    }

    private variable _win ""       ;# containing frame
}
