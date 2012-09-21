# ----------------------------------------------------------------------
#  P2P: options
#
#  This code manages option settings within all of the programs in the
#  peer to peer job management system.  Options are soft-coded
#  parameters controlling things like how many peers each node should
#  be connected to.
# ----------------------------------------------------------------------
#  Michael McLennan (mmclennan@purdue.edu)
# ======================================================================
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Rappture

namespace eval p2p {
    variable options  ;# array of option settings for this program
}

# ======================================================================
#  OPTIONS
#  These options get the worker started, but new option settings
#  some from the authority after this worker first connects.
# ----------------------------------------------------------------------
#  USAGE:  p2p::options register <name> <defaultValue>
#  USAGE:  p2p::options set <name> <newValue>
#  USAGE:  p2p::options get <name>
#
#  Use these commands to register and get/set various option settings
#  used within these programs.  The option settings start with default
#  values but can be overridden at any point, for example by the
#  "authority" program.
# ======================================================================
proc p2p::options {op args} {
    global options

    switch -- $op {
        register {
            if {[llength $args] != 2} {
                error "wrong # args: should be \"p2p::options register name default\""
            }
            set opt [lindex $args 0]
            set def [lindex $args 1]
            if {[info exists options($opt)]} {
                error "option \"$opt\" is already registered"
            }
            set options($opt) $def
        }
        get {
            if {[llength $args] != 1} {
                error "wrong # args: should be \"p2p::options get name\""
            }
            set opt [lindex $args 0]
            if {![info exists options($opt)]} {
                error "option \"$opt\" is not defined"
            }
            return $options($opt)
        }
        set {
            if {[llength $args] != 2} {
                error "wrong # args: should be \"p2p::options set name default\""
            }
            set opt [lindex $args 0]
            set val [lindex $args 1]
            if {![info exists options($opt)]} {
                error "option \"$opt\" is not defined"
            }
            set options($opt) $val
        }
    }
}
