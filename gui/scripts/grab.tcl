# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: grab - improved version of the Tk grab command
#
#  This version of "grab" keeps a stack of grab windows, so one
#  window can steal and release the grab, and the grab will revert
#  to the previous window.  If things get jammed up, you can press
#  <Escape> three times to release the grab.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require BLT

namespace eval Rappture { # forward declaration }
namespace eval Rappture::grab {
    variable state ""  ;# local ("") or global ("-global") grab
    variable stack ""  ;# stack of grab windows
}

proc Rappture::grab::init {} { # used for autoloading this module }

bind all <Escape><Escape><Escape> Rappture::grab::reset

# ----------------------------------------------------------------------
# USAGE: grab ?-global? <window>
# USAGE: grab set ?-global? <window>
# USAGE: grab release <window>
# USAGE: grab current ?<window>?
# USAGE: grab status <window>
#
# This is a replacement for the usual Tk grab command.  It works
# exactly the same way, but supports a stack of grab windows, so
# one window can steal grab from another, and then give it back
# later.
# ----------------------------------------------------------------------
rename grab _tk_grab
proc grab { args } {
    set op [lindex $args 0]
    if {[winfo exists $op]} {
        set op "set"
    } elseif {$op == "-global" && [winfo exists [lindex $args end]]} {
        set op "set"
    }

    if {$op == "set"} {
        #
        # Handle GRAB SET specially.
        # Add the new grab window to the grab stack.
        #
        set state $::Rappture::grab::state
        set window [lindex $args end]

        if {[lsearch -exact $args -global] >= 0} {
            set state "-global"
        }

        if {"" != $state} {
            # if it's a global grab, store the -global flag away for later
            set window [linsert $window 0 $state]

            # all grabs from now on are global
            set ::Rappture::grab::state "-global"
        }

        # if the window is already on the stack, then skip it
        if {[string equal [lindex $::Rappture::grab::stack 0] $window]} {
            return $window
        }

        # add the current configuration to the grab stack
        set ::Rappture::grab::stack \
            [linsert $::Rappture::grab::stack 0 $window]

        return [eval _grabset $window]

    } elseif {$op == "release"} {
        #
        # Handle GRAB RELEASE specially.
        # Release the current grab and grab the next window on the stack.
        # Note that the current grab is on the top of the stack.  The
        # next one down is the one we want to revert to.
        #
        set window [lindex $::Rappture::grab::stack 1]
        set ::Rappture::grab::stack [lrange $::Rappture::grab::stack 1 end]

        # release the current grab
        eval _tk_grab $args

        # and set the next one
        if {[lindex $window 0] != "-global"} {
            # no more global grabs -- resume local grabs
            set ::Rappture::grab::state ""
        }
        if { $window != "" } {
            eval _grabset $window
        }
        return ""
    }

    # perform any other grab operation as usual...
    return [eval _tk_grab $args]
}

proc _grabset {args} {
    # give it 3 tries, if necessary
    for {set i 0} {$i < 3} {incr i} {
        set status [catch {eval _tk_grab set $args} result]
        if {$status == 0} {
            return $result
        }
        after 100; update
    }
    # oh well, we tried...
    return ""
}

# ----------------------------------------------------------------------
# USAGE: Rappture::grab::reset
#
# Used internally to reset the grab whenever the user presses
# Escape a bunch of times to break out of the grab.
# ----------------------------------------------------------------------
proc Rappture::grab::reset {} {
    set w [_tk_grab current]
    if {"" != $w} {
        _tk_grab release $w
    }
    set Rappture::grab::stack ""
    set Rappture::grab::state ""

    foreach win [blt::busy windows] {
        blt::busy release $win
    }
}
