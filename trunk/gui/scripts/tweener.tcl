# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: tweener - used for animating smooth movements
#
#  Each Tweener executes a command repeatedly through a series of
#  values from one extreme to another.  For example, the command may
#  change the size of a rectangle, and the values may go from 10 to
#  200.  The Tweener will make that happen over a specified interval
#  of time.  The animation can be stopped or changed at any point,
#  and if the Tweener is destroyed, the action is cancelled.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itcl

itcl::class Rappture::Tweener {
    public variable from 0        ;# value goes from this...
    public variable to   1        ;# ...to this
    public variable duration 500  ;# during this time interval
    public variable steps 10      ;# with this many steps
    public variable command ""    ;# gets executed with %v for value
    public variable finalize ""   ;# gets executed at the end to finish up

    constructor {args} { eval configure $args }
    destructor { # defined below }

    public method go {{how "-resume"}}
    public method stop {}

    private method _next {}

    private variable _nstep 0      ;# current step number
    private variable _afterid ""   ;# pending after event
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Tweener::destructor {} {
    stop
}

# ----------------------------------------------------------------------
# USAGE: go ?-resume|-restart?
#
# Causes the current animation to start running, either where it
# left off (-resume) or fromt the beginning (-restart).
# ----------------------------------------------------------------------
itcl::body Rappture::Tweener::go {{how -resume}} {
    switch -- $how {
        -resume {
            # leave _nstep alone
        }
        -restart {
            set _nstep 0
        }
        default {
            error "bad option \"$how\": should be -restart or -resume"
        }
    }
    stop
    set _afterid [after idle [itcl::code $this _next]]
}

# ----------------------------------------------------------------------
# USAGE: stop
#
# Causes the animation to stop by cancelling any pending after event.
# ----------------------------------------------------------------------
itcl::body Rappture::Tweener::stop {} {
    if {"" != $_afterid} {
        after cancel $_afterid
        set _afterid ""
    }
}

# ----------------------------------------------------------------------
# USAGE: _next
#
# Used internally to advance the animation.  Executes the -command
# option with the current value substituted in place of any %v values.
# ----------------------------------------------------------------------
itcl::body Rappture::Tweener::_next {} {
    set value [expr {($to-$from)/double($steps)*$_nstep + $from}]
    set cmd $command
    regsub -all %v $cmd $value cmd
    if {[catch {uplevel #0 $cmd} result]} {
        bgerror "$result\n    (while managing tweener $this)"
    }

    if {[incr _nstep] <= $steps} {
        set delay [expr {round($duration/double($steps))}]
        set _afterid [after $delay [itcl::code $this _next]]
    } elseif {[string length $finalize] > 0} {
        set cmd $finalize
        regsub -all %v $cmd $value cmd
        if {[catch {uplevel #0 $cmd} result]} {
            bgerror "$result\n    (while finalizing tweener $this)"
        }
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -command
# ----------------------------------------------------------------------
itcl::configbody Rappture::Tweener::command {
    if {[string length $command] == 0} {
        stop
    }
}
