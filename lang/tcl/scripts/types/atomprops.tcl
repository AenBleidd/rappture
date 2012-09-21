# ----------------------------------------------------------------------
#  EDITOR: atom property attribute values
#
#  Used within the Instant Rappture builder to edit the values reported
#  by a <periodicelement> object.  Includes things like atomic number,
#  weight, etc.
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
itcl::class AttrAtomprops {
    constructor {win args} {
        Rappture::getopts args params {
            value -tooltip ""
        }

        Rappture::Combochecks $win.val
        pack $win.val -side left -expand yes -fill x
        $win.val choices insert end \
            "name" "Element Name" \
            "symbol" "Atomic Symbol" \
            "number" "Atomic Number" \
            "weight" "Atomic Weight"

        $win.val value "name"

        Rappture::Tooltip::for $win.val $params(-tooltip)

        set _win $win
    }

    public method check {} {
        set val [$_win.val value]
        if {$val eq ""} {
            return [list error "Need to report something for the selected elment."]
        }
        return ""
    }

    public method load {val} {
        # ignore comma separators
        regsub -all {,} $val { } val

        # having a null value is an error -- default to name
        if {$val eq ""} {
            set val "name"
        }
        if {[catch {$_win.val value $val}]} {
            $_win.val value "name"
        }
    }

    public method save {var} {
        upvar $var value
        set value [$_win.val value]
        return 1
    }

    public method edit {} {
        focus -force $_win.val
    }

    public proc import {xmlobj path} {
        set val [$xmlobj get $path]
        if {$val eq ""} {
            set val "name"
        } elseif {$val eq "all"} {
            set val "name symbol number weight"
        }
        return $val
    }

    public proc export {xmlobj path value} {
        $xmlobj put $path $value
    }

    private variable _win ""       ;# containing frame
}
