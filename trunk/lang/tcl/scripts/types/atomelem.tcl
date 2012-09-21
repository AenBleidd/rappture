# ----------------------------------------------------------------------
#  EDITOR: atomic element type
#
#  Used within the Instant Rappture builder to edit the default value
#  for a <periodicelement> object.  Uses the PeriodicTable itself to
#  select the element.
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
itcl::class AttrAtomelem {
    constructor {win args} {
        Rappture::getopts args params {
            value -tooltip ""
        }

        Rappture::PeriodicElement $win.val -editable no
        pack $win.val -side left -expand yes -fill x

        Rappture::Tooltip::for $win.val $params(-tooltip)

        set _win $win
    }

    public method check {} {
        # any value here is okay -- check at the object level
        return ""
    }

    public method load {val} {
        $_win.val value $val
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
        $xmlobj get $path
    }

    public proc export {xmlobj path value} {
        $xmlobj put $path $value
    }

    private variable _win ""       ;# containing frame
}
