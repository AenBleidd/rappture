# ----------------------------------------------------------------------
#  EDITOR: units attribute values
#
#  Used within the Instant Rappture builder to edit units for numbers.
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
itcl::class AttrUnits {
    constructor {win args} {
        Rappture::getopts args params {
            value -tooltip ""
        }

        Rappture::Combobox $win.val -width 10 -editable yes
        pack $win.val -side left
        $win.val choices insert end \
          m   "Length Units" \
          m   "  m - meters" \
          in  "  in - inches" \
          ft  "  ft - feet" \
          yd  "  yd - yards" \
          mi  "  mi - miles" \
          s   "Time Units" \
          s   "  s - seconds" \
          min "  min - minutes" \
          h   "  h - hours" \
          d   "  d - days" \
          Hz  "  Hz - Hertz" \
          K   "Temperature Units" \
          K   "  K - Kelvin" \
          F   "  F - Fahrenheit" \
          C   "  C - Celcius" \
          eV  "Energy Units" \
          eV  "  eV - electron Volts" \
          J   "  J - Joules" \
          V   "  V - volts" \
          m3  "Volume Units" \
          m3  "  cubic meter" \
          gal "  gal - US gallon" \
          L   "  L - liter" \
          deg "Angle Units" \
          deg "  deg - degrees" \
          rad "  rad - radians" \
          g   "Mass Units" \
          g   "  g - grams" \
          N   "Force Units" \
          N   "  N - Newtons" \
          Pa  "Pressure Units" \
          Pa  "  Pa - Pascals" \
          atm "  atm - atmospheres" \
          pis "  psi - pounds per square inch" \
          T   "Magnetic Units" \
          T   "  T - Teslas" \
          G   "  G - Gauss"

        Rappture::Tooltip::for $win.val $params(-tooltip)

        set _win $win
    }

    public method check {} {
        set val [$_win.val translate [$_win.val value]]
        if {"" != $val} {
            return ""
        }
        set val [$_win.val value]
        if {"" != [Rappture::Units::description $val]} {
            return ""
        }
        return [list error "Bad value \"$val\": should be valid system of units"]
    }

    public method load {val} {
        set newval [$_win.val translate $val]
        if {"" != $newval} {
            $_win.val value $newval
        } else {
            $_win.val value $val
        }
    }

    public method save {var} {
        upvar $var value

        set err [lindex [check] 1]
        if {[string length $err] > 0} {
            Rappture::Tooltip::cue $_win $err
            return 0
        }

        set val [$_win.val value]
        set newval [$_win.val translate $val]
        if {"" != $newval} {
            set value $newval
        } else {
            set value $val
        }
        return 1
    }

    public method edit {} {
        focus -force $_win.val
        $_win.val component entry selection from 0
        $_win.val component entry selection to end
    }

    public proc import {xmlobj path} {
        set val [$xmlobj get $path]
        return $val
    }

    public proc export {xmlobj path value} {
        set value [string trim $value]
        if {$value ne ""} {
            $xmlobj put $path $value
        }
    }

    private variable _win ""       ;# containing frame
}
