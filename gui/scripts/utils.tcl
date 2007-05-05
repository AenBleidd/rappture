# ----------------------------------------------------------------------
#  COMPONENT: utils - miscellaneous utilities
#
#  Misc routines used throughout the GUI.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
namespace eval Rappture { # forward declaration }
namespace eval Rappture::utils { # forward declaration }

# ----------------------------------------------------------------------
# USAGE: hexdump ?-lines num? <data>
#
# Returns a hex dump for a blob of binary <data>.  This is used to
# represent the data in an input/output <string> object.  The -lines
# flag can be used to limit the output to the specified number of
# lines.
# ----------------------------------------------------------------------
proc Rappture::utils::hexdump {args} {
    Rappture::getopts args params {
        value -lines unlimited
    }
    if {[llength $args] != 1} {
        error "wrong # args: should be \"hexdump ?-lines num? data\""
    }
    set newval [lindex $args 0]
    set args ""

    set size [string length $newval]
    foreach {factor units} {
        1073741824 GB
        1048576 MB
        1024 kB
        1 bytes
    } {
        if {$size/$factor > 0} {
            if {$factor > 1} {
                set size [format "%.2f" [expr {double($size)/$factor}]]
            }
            break
        }
    }

    set rval "<binary> $size $units"

    if {$params(-lines) != "unlimited" && $params(-lines) <= 0} {
        return $rval
    }

    append rval "\n\n"
    set len [string length $newval]
    for {set i 0} {$i < $len} {incr i 8} {
        append rval [format "%#06x: " $i]
        set ascii ""
        for {set j 0} {$j < 8} {incr j} {
            if {$i+$j < $len} {
                set char [string index $newval [expr {$i+$j}]]
                binary scan $char c ichar
                set hexchar [format "%02x" [expr {0xff & $ichar}]]
            } else {
                set char " "
                set hexchar "  "
            }
            append rval "$hexchar "
            if {[regexp {[\000-\037\177-\377]} $char]} {
                append ascii "."
            } else {
                append ascii $char
            }
        }
        append rval " | $ascii\n"

        if {"unlimited" != $params(-lines) && $i/8+1 >= $params(-lines)} {
            if {$i < $len-1} {
                append rval "more..."
            }
            break
        }
    }
    return $rval
}
