# ----------------------------------------------------------------------
#  LIBRARY: statemachine
#
#  This object comes in handy when building the state machines that
#  drive the behavior of clients and servers in the peer-to-peer system.
#  Each state machine has a number of recognized states and trasitions
#  between the states that kick off actions within the system.
# ----------------------------------------------------------------------
#  Michael McLennan (mmclennan@purdue.edu)
# ======================================================================
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itcl

# ======================================================================
#  CLASS: StateMachine
# ======================================================================
itcl::class StateMachine {
    private variable _states  ;# maps states to -enter/-leave commands
    private variable _trans   ;# maps s1->s2 transitions to commands
    private variable _current ""
    private variable _slots   ;# maps statedata name => value
    private variable _intrans 0

    constructor {args} {
        eval configure $args
        set _states(all) ""
    }

    public method state {name options}
    public method transition {name options}
    public method current {} { return $_current }
    public method all {} { return $_states(all) }
    public method goto {state}
    public method statedata {args}
}

# ----------------------------------------------------------------------
#  USAGE: state <name> ?-option value -option value ...?
#
#  Defines a new state in the state machine.  Recognizes the following
#  options:
#    -onenter ... command invoked when state is entered
#    -onleave ... command invoked when state is left
# ----------------------------------------------------------------------
itcl::body StateMachine::state {name args} {
    array set options {
        -onenter ""
        -onleave ""
    }
    foreach {key val} $args {
        if {![info exists options($key)]} {
            error "bad option \"$key\": should be [join [lsort [array names options]] {, }]"
        }
        set options($key) $val
    }

    if {![regexp {^[-a-zA-Z0-9_]+$} $name]} {
        error "bad state name \"$name\": should be alphanumeric, including - or _"
    }

    if {[lsearch $_states(all) $name] >= 0} {
        error "state \"$name\" already defined"
    }
    lappend _states(all) $name
    set _states($name-onenter) $options(-onenter)
    set _states($name-onleave) $options(-onleave)

    # start in the first state by default
    if {$_current == ""} {
        goto $name
    }
    return $name
}

# ----------------------------------------------------------------------
#  USAGE: transition <name> ?-option value -option value ...?
#
#  Defines the transition from one state to another.  The transition
#  <name> should be of the form "s1->s2", where "s1" and "s2" are
#  recognized state names.  Recognizes the following options:
#    -onchange ... command invoked when transition is followed
# ----------------------------------------------------------------------
itcl::body StateMachine::transition {name args} {
    array set options {
        -onchange ""
    }
    foreach {key val} $args {
        if {![info exists options($key)]} {
            error "bad option \"$key\": should be [join [lsort [array names options]] {, }]"
        }
        set options($key) $val
    }

    if {![regexp {^([-a-zA-Z0-9_]+)->([-a-zA-Z0-9_]+)$} $name match state1 state2]} {
        error "bad transition name \"$name\": should have the form s1->s2"
    }
    if {[lsearch $_states(all) $state1] < 0} {
        error "unrecognized starting state \"$state1\" for transition"
    }
    if {[lsearch $_states(all) $state2] < 0} {
        error "unrecognized ending state \"$state2\" for transition"
    }

    set _trans($state1->$state2-onchange) $options(-onchange)
    return $name
}

# ----------------------------------------------------------------------
#  USAGE: goto <name>
#
#  Forces the state machine to undergo a transition from the current
#  state to the new state <name>.  If the current state has a -onleave
#  hook, it is executed first.  If there is a transition between the
#  states with a -onchange hook, it is executed next.  If the new
#  state has a -onenter hook, it is executed last.  If any of the
#  command hooks along the way fail, then the state machine will
#  drop back to its current state and return the error.
# ----------------------------------------------------------------------
itcl::body StateMachine::goto {state} {
    if {$_intrans} {
        error "can't change states while making a transition"
    }

    #
    # NOTE: Use _'s for all local variables in this routine.  We eval
    #   the code fragments or -onenter, -onleave, -onchange directly
    #   in the context of this method.  This keeps them from polluting
    #   the global scope and gives them access to the "statedata"
    #   method.  But it could have side effects if the code fragments
    #   accidentally redefined local variables.  So start all local
    #   variables with _ to avoid any collisions.
    #
    set _sname $state

    if {[lsearch $_states(all) $_sname] < 0} {
        error "bad state name \"$_sname\": should be one of [join [lsort $_states(all)] {, }]"
    }
    set _intrans 1

    # execute any -onleave hook for the current state
    set _goback 0
    if {"" != $_current && "" != $_states($_current-onleave)} {
        if {[catch $_states($_current-onleave) _result]} {
            set _goback 1
        }
    }

    # execute any -onchange hook for the transition
    if {!$_goback && [info exists _trans($_current->$_sname-onchange)]} {
        if {[catch $_trans($_current->$_sname-onchange) _result]} {
            set _goback 1
        }
    }

    # execute any -onenter hook for the new state
    if {!$_goback && "" != $_states($_sname-onenter)} {
        if {[catch $_states($_sname-onenter) _result]} {
            set _goback 1
        }
    }

    if {$_goback} {
        if {"" != $_current && "" != $_states($_current-onenter)} {
            catch $_states($_current-onenter)
        }
        set _intrans 0
        error $_result "$_result\n    (while transitioning from $_current to $_sname)"
    }

    set _intrans 0
    set _current $_sname
    return $_current
}

# ----------------------------------------------------------------------
#  USAGE: statedata ?<name>? ?<value>?
#
#  Used to get/set special data values stored within the state
#  machine.  These are like local variables, but can be shared
#  across the code fragments associated with states and transitions.
#  We could also use global variables for this, but this allows
#  each state machine to store its own values.
# ----------------------------------------------------------------------
itcl::body StateMachine::statedata {args} {
    switch [llength $args] {
        0 {
            return [array get _slots]
        }
        1 {
            set name [lindex $args 0]
            if {[info exists _slots($name)]} {
                return $_slots($name)
            }
            return ""
        }
        2 {
            set name [lindex $args 0]
            set value [lindex $args 1]
            set _slots($name) $value
            return $value
        }
        default {
            error "wrong # args: should be \"statedata ?name? ?value?\""
        }
    }
}
