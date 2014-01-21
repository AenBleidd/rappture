# ----------------------------------------------------------------------
#  EDITOR: string attribute values
#
#  Used within the Instant Rappture builder to edit string values.
#  Strings can have the following additional options:
#
#    -lines N ......... height of editor (default is 1 line)
#    -validate type ... validation routine applied to string values
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
itcl::class AttrString {
    constructor {win args} {
        Rappture::getopts args params {
            value -lines 1
            value -validate ""
            value -tooltip ""
        }

        if {$params(-lines) > 1} {
            Rappture::Scroller $win.scrl -xscrollmode none -yscrollmode auto
            pack $win.scrl -expand yes -fill both
            text $win.scrl.text -width 10 -height $params(-lines) -wrap word
            $win.scrl contents $win.scrl.text
            Rappture::Tooltip::for $win.scrl.text $params(-tooltip)
        } else {
            entry $win.str
            pack $win.str -fill x
            Rappture::Tooltip::for $win.str $params(-tooltip)
        }
        set _win $win
        set _validate $params(-validate)
    }

    public method load {val} {
        if {[winfo exists $_win.str]} {
            $_win.str delete 0 end
            $_win.str insert end $val
        } else {
            $_win.scrl.text delete 1.0 end
            $_win.scrl.text insert end $val
        }
    }

    public method check {} {
        if {[string length $_validate] > 0} {
            if {[winfo exists $_win.str]} {
                set str [$_win.str get]
            } else {
                set str [$_win.scrl.text get 1.0 end-1char]
            }
            if {[catch {uplevel #0 $_validate [list $str]} result]} {
                return [list error "Bad value: $result"]
            }
        }
    }

    public method save {var} {
        upvar $var value

        set err [lindex [check] 1]
        if {[string length $err] > 0} {
            Rappture::Tooltip::cue $_win $err
            return 0
        }

        if {[winfo exists $_win.str]} {
            set value [$_win.str get]
        } else {
            set value [$_win.scrl.text get 1.0 end-1char]
        }
        return 1
    }

    public method edit {} {
        if {[winfo exists $_win.str]} {
            focus -force $_win.str
            $_win.str selection from 0
            $_win.str selection to end
        } else {
            focus -force $_win.scrl.text
            $_win.scrl.text tag add sel 1.0 end
        }
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

    private variable _win ""       ;# containing frame
    private variable _validate ""  ;# validation command for checking values
}
