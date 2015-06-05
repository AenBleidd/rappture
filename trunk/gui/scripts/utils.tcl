# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: utils - miscellaneous utilities
#
#  Misc routines used throughout the GUI.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
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

    set rval "<binary> [Rappture::utils::binsize [string length $newval]]"

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

# ----------------------------------------------------------------------
# USAGE: binsize <length>
#
# Returns a user-friendly expression of data size, like "12 kB" or
# "144 MB".
# ----------------------------------------------------------------------
proc Rappture::utils::binsize {size} {
    foreach {factor units} {
        1073741824 GB
        1048576 MB
        1024 kB
        1 bytes
    } {
        if {$size/$factor > 0} {
            if {$factor > 1} {
                set size [format "%.1f" [expr {double($size)/$factor}]]
            }
            break
        }
    }
    return "$size $units"
}

# ----------------------------------------------------------------------
# USAGE: datatype <binary>
#
# Examines the given <binary> string and returns a description of
# the data format.
# ----------------------------------------------------------------------
proc Rappture::utils::datatype {binary} {
    set fileprog [auto_execok file]
    if {[string length $binary] == 0} {
        set desc "Empty"
    } elseif {"" != $fileprog} {
        #
        # Use Unix "file" program to get info about type
        # HACK ALERT! must send binary data in by creating a tmp file
        #   or else it gets corrupted and misunderstood
        #
        set id [pid]
        while {[file exists /tmp/datatype$id]} {
            incr id
        }
        set fname "/tmp/datatype$id"
        set fid [open $fname w]
        fconfigure $fid -translation binary -encoding binary
        puts -nonewline $fid [string range $binary 0 1024]
        close $fid
        if {[catch {exec $fileprog -b $fname} desc]} {
            set desc "Binary data"
        }
        catch {file delete $fname}
    } else {
        set desc "Binary data"
    }
    return $desc
}

# ----------------------------------------------------------------------
# USAGE: expandPath <path>
#
# Returns the true location of the provided path,
# automatically expanding links to form an absolute path.
# ----------------------------------------------------------------------
proc Rappture::utils::expandPath {args} {
    set path ""
    set dirs [file split [lindex $args 0]]

    while {[llength $dirs] > 0} {
        set d [lindex $dirs 0]
        set dirs [lrange $dirs 1 end]
        if {[catch {file link [file join $path $d]} out] == 0} {
            # directory d is a link, follow it
            set outdirs [file split $out]
            if {[string compare "/" [lindex $outdirs 0]] == 0} {
                # directory leads back to root
                # clear path
                # reset dirs list
                set path ""
                set dirs $outdirs
            } else {
                # relative path for the link
                # prepend directory to dirs list
                set dirs [concat $outdirs $dirs]
            }
        } else {
            set path [file join $path $d]
        }
    }
    return $path
}

