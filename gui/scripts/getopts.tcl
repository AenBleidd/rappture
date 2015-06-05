# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: getopts - simple argument parser
#
#  This utility makes it easy to process arguments for various
#  commands.  You give a description of arguments, and it will
#  parse and return the values.
#
#    getopts args params {
#        value -foo foo_default
#        flag group -bar default
#        flag group -baz
#    }
#
#  Note that both args and params are passed by name, not by
#  value (not $args or $params).  This function initializes the
#  params variable according to the values in the spec list.
#  It then loops through all args, matching as many as possible
#  from the list.  It throws an error on the first error
#  encountered.  If there are extra arguments that don't match
#  the flags and don't start with -, they are left in args, and
#  processing stops.
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
namespace eval Rappture { # forward declaration }

# ----------------------------------------------------------------------
# USAGE: getopts <listvar> <returnvar> <spec>
#
# Processes options in <listvar>, storing results in <returnvar>.
# Throws an exception if an error is encountered.  Leaves any remaining
# arguments (after flags) in <listvar>.
# ----------------------------------------------------------------------
proc Rappture::getopts {listVar returnVar spec} {
    upvar $listVar args
    upvar $returnVar params
    catch {unset params}
    set opts ""

    #
    # Pick apart the info in the <spec> and set up flags/params
    #
    foreach line [split $spec \n] {
        if {[llength $line] == 0} {
            continue  ;# ignore blank lines
        }

        set type [lindex $line 0]
        switch -- $type {
            value {
                if {[llength $line] < 3} {
                    error "bad value spec \"$line\": should be \"value -flag default\""
                }
                set name [lindex $line 1]
                set flags($name) $type
                set params($name) [lindex $line 2]
                lappend opts $name
            }
            flag {
                if {[llength $line] < 3 || [llength $line] > 4} {
                    error "bad value spec \"$line\": should be \"flag group -flag ?default?\""
                }
                set group [lindex $line 1]
                set name [lindex $line 2]
                set flags($name) [list $type $group]
                if {[llength $line] > 3} {
                    set params($group) $name
                    set params($name) 1
                } else {
                    if {![info exists params($group)]} {
                        set params($group) ""
                    }
                    set params($name) 0
                }
                lappend opts $name
            }
            list {
                if {[llength $line] < 3} {
                    error "bad value spec \"$line\": should be \"list -flag default\""
                }
                set name [lindex $line 1]
                set flags($name) $type
                set params($name) [lindex $line 2]
                lappend opts $name
            }
            default {
                error "bad arg type \"$type\": should be flag or value"
            }
        }
    }

    #
    # Now, walk through the values in $args and extract parameters.
    #
    while {[llength $args] > 0} {
        set first [lindex $args 0]
        if {[string index $first 0] != "-"} {
            break
        }
        if {"--" == $first} {
            set args [lrange $args 1 end]
            break
        }
        if {![info exists params($first)]} {
            error "bad option \"$first\": should be [join [lsort $opts] {, }]"
        }
        switch -- [lindex $flags($first) 0] {
            value {
                if {[llength $args] < 2} {
                    error "missing value for option $first"
                }
                set params($first) [lindex $args 1]
                set args [lrange $args 2 end]
            }
            flag {
                set group [lindex $flags($first) 1]
                set params($group) $first
                set params($first) 1
                set args [lrange $args 1 end]
            }
            list {
                if {[llength $args] < 2} {
                    error "missing value for option $first"
                }
                foreach arg [lrange $args 1 end] {
                    if {[string index $arg 0] == "-"} {
                        break
                    }
                }
                set idx [lsearch -exact $args $arg]
                if {$idx == [expr [llength $args] - 1]} {
                    # reached the end of the $args list
                    # with no other -'d arguments
                    set params($first) [lrange $args 1 end]
                    set args ""
                } else {
                    # there are further -'d arguments to process
                    set params($first) [lrange $args 1 [expr $idx-1]]
                    set args [lrange $args $idx end]
                }
            }
        }
    }
    return ""
}
