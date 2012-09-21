# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <enable> attributes
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Rappture

# open the XML file containing the run parameters
set driver [Rappture::library [lindex $argv 0]]

set result ""

set model [$driver get input.(model).current]
switch -- $model {
    dd {
        append result "Drift-Diffusion:\n"
        set recomb [$driver get input.(dd).(recomb).current]
        append result "  Recombination model: $recomb\n"
        if {$recomb} {
            set taun [$driver get input.(dd).(taun).current]
            set taup [$driver get input.(dd).(taup).current]
            append result "  TauN: $taun\n"
            append result "  TauP: $taup\n"
        }
    }
    bte {
        append result "Boltzmann Transport Equation:\n"
        set temp [$driver get input.(bte).(temp).current]
        append result "  Temperature: $temp\n"
        set secret [$driver get input.(bte).(secret).current]
        append result "  Hidden number: $secret\n"
    }
    negf {
        append result "NEGF Analysis:\n"
        set tbe [$driver get input.(negf).(tbe).current]
        append result "  Tight-binding energy: $tbe\n"
        set tau [$driver get input.(negf).(tau).current]
        append result "  High-energy lifetime: $tau\n"
    }
    default {
        error "can't understand $model"
    }
}

$driver put output.log $result

# save the updated XML describing the run...
Rappture::result $driver
exit 0
