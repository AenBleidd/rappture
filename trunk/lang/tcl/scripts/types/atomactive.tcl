# ----------------------------------------------------------------------
#  EDITOR: atom active/inactive settings
#
#  Used within the Instant Rappture builder to edit the active/inactve
#  property used by a <periodicelement> object.  This determines what
#  parts of the periodic table are allowed or forbidden.
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
itcl::class AttrAtomactive {
    constructor {win args} {
        Rappture::getopts args params {
            value -tooltip ""
        }

        Rappture::Combobox $win.how -width 17 -editable no
        pack $win.how -side left
        $win.how choices insert end \
            "all" "All elements" \
            "active" "Only these:" \
            "inactive" "Everything except:"

        bind $win.how <<Value>> [itcl::code $this _react]
        $win.how value [$win.how label "all"]

        Rappture::Combochecks $win.which
        $win.which choices insert end \
            "actinoid" "Actinoid" \
            "alkali-metal" "Alkali metal" \
            "alkaline-earth-metal" "Alkaline earth metal" \
            "halogen" "Halogen" \
            "lanthanoid" "Lanthanoid" \
            "metalloid" "Metalloid" \
            "noble-gas" "Noble gas" \
            "other-non-metal" "Other non-metal" \
            "post-transition-metal" "Post-transition metal" \
            "transition-metal" "Transition metal" \
            "unknown" "Unknown"

        Rappture::Tooltip::for $win.how $params(-tooltip)
        Rappture::Tooltip::for $win.which $params(-tooltip)

        set _win $win
    }

    public method check {} {
        # user can't enter anything bad
        return ""
    }

    public method load {val} {
        if {[regexp {^(active|inactive): (.+)} $val match which vlist]} {
            $_win.how value [$_win.how label $which]
            $_win.which value [split $vlist ,]
        } elseif {$val eq ""} {
            $_win.how value [$_win.how label "all"]
        }
        _react
    }

    public method save {var} {
        upvar $var value
        switch -- [$_win.how translate [$_win.how value]] {
            active {
                set value "active: [join [$_win.which value] ,]"
            }
            inactive {
                set value "inactive: [join [$_win.which value] ,]"
            }
            default {
                set value ""
            }
        }
        return 1
    }

    public method edit {} {
        focus -force $_win.how
    }

    public proc import {xmlobj path} {
        # attribute is associated with .active, but we need to look
        # at both .active and .inactive
        set path [join [lrange [split $path .] 0 end-1] .]

        set aval [string trim [$xmlobj get $path.active]]
        set ival [string trim [$xmlobj get $path.inactive]]
        if {$aval ne ""} {
            set val "active: [join $aval ,]"
        } elseif {$ival ne ""} {
            set val "inactive: [join $ival ,]"
        } else {
            set val ""
        }
        return $val
    }

    public proc export {xmlobj path value} {
        # attribute is associated with .active, but we need to look
        # at both .active and .inactive
        set path [join [lrange [split $path .] 0 end-1] .]

        if {[regexp {^(active|inactive): (.+)} $value match which vlist]} {
            $xmlobj put $path.$which [split $vlist ,]
        }
    }

    private method _react {} {
        if {[$_win.how translate [$_win.how value]] eq "all"} {
            pack forget $_win.which
        } else {
            pack $_win.which -side left -expand yes -fill x
        }
    }

    private variable _win ""       ;# containing frame
}
