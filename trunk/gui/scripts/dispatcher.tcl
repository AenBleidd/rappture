# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: dispatcher - central notification mechanism
#
#  The DispatchObj is used within other objects to manage events that
#  the object sends to itself or other clients.  Each event type
#  must first be registered via the "register" method.  After that,
#  an event can be dispatched by calling the "event" method.  Clients
#  can bind to the various event types by registering a callback via
#  the "dispatch" method.
#
#  The Dispatcher base class provides a foundation for objects that
#  use the DispatchObj internally.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itcl

namespace eval Rappture { # forward declaration }

proc Rappture::dispatcher {varName} {
    upvar $varName obj
    set obj [Rappture::DispatchObj ::#auto]
    trace variable obj u "[list rename $obj {}]; list"
}

itcl::class Rappture::DispatchObj {
    constructor {} { # defined below }
    destructor { # defined below }

    public method register {args}
    public method dispatch {option args}
    public method event {name args}
    public method cancel {args}
    public method ispending {name}

    protected method _send {event caller {arglist ""}}

    private variable _event2clients  ;# maps !event => clients
    private variable _event2datacb   ;# maps !event => data callback
    private variable _dispatch       ;# maps !event/client => callback
    private variable _extraargs      ;# extra args for dispatch calls
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::DispatchObj::constructor {} {
    # nothing to do
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::DispatchObj::destructor {} {
    eval cancel [array names _event2clients]
}

# ----------------------------------------------------------------------
# USAGE: register !event ?dataCallback?
#
# Clients use this to register new event types with this dispatcher.
# Once registered, each !event name can be used in the "dispatch"
# method to bind to the event, and in the "event" method to raise
# the event.  If the optional dataCallback is supplied, then it is
# invoked whenever the event is triggered to generate event data
# that gets passed along to the various dispatch callbacks.
# ----------------------------------------------------------------------
itcl::body Rappture::DispatchObj::register {name {datacb ""}} {
    if {![regexp {^![-_a-zA-Z0-9]} $name]} {
        error "bad name \"$name\": should be !name"
    }
    if {[info exists _event2clients($name)]} {
        error "event \"$name\" already exists"
    }
    set _event2clients($name) ""
    set _event2datacb($name) $datacb
}

# ----------------------------------------------------------------------
# USAGE: dispatch caller ?-now? ?!event ...? ?callback?
#
# Clients use this to query binding information, or to bind a callback
# to one or more of the !events recognized by this object.
#
# With no args, this returns a list of all events that are currently
# bound to the caller.
#
# With one argument, this returns the callback associated with the
# specified !event for the given caller.
#
# Otherwise, it sets the callback for one or more events.  If the
# -now flag is specified, then the callback is invoked immediately
# to send data back to this caller.  This comes in handy when the
# caller is set up to react to changes.  It provides a way of
# initializing the client, as if an event had just occurred.
# ----------------------------------------------------------------------
itcl::body Rappture::DispatchObj::dispatch {caller args} {
    if {[llength $args] == 0} {
        # no args? then return a list of events for this caller
        foreach key [array names _dispatch $caller-*] {
            set name [lindex [split $key -] 1]
            set events($name) 1
        }
        return [array names events]
    } elseif {[llength $args] == 1} {
        set name [lindex $args 0]
        if {[info exists _dispatch($caller-$name)]} {
            return $_dispatch($caller-$name)
        }
        return ""
    }

    # set a callback for one or more events
    set now 0
    set events ""
    set callback [lindex $args end]
    foreach str [lrange $args 0 end-1] {
        if {$str == "-now"} {
            set now 1
        } elseif {[info exists _event2clients($str)]} {
            lappend events $str
        } else {
            if {[string index $str 0] == "-"} {
                error "bad option \"$str\": should be -now"
            } else {
                error "bad event \"$str\": should be [join [lsort [array names _event2clients]] {, }]"
            }
        }
    }

    #
    # If the callback is "", then remove the callback for this
    # caller.  Otherwise, set the callback for this caller.
    #
    foreach name $events {
        cancel $name

        if {"" == $callback} {
            catch {unset _dispatch($caller-$name)}
            if {"" == [array names _dispatch $caller-*]} {
                set i [lsearch $_event2clients($name) $caller]
                if {$i >= 0} {
                    set _event2clients($name) [lreplace $_event2clients($name) $i $i]
                }
            }
        } else {
            set _dispatch($caller-$name) $callback
            set i [lsearch $_event2clients($name) $caller]
            if {$i < 0} { lappend _event2clients($name) $caller }

            if {$now} {
                _send $name $caller
            }
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: event ?-now? ?-later? ?-idle? ?-after time? !event ?args...?
#
# Clients use this to dispatch an event to any callers who have
# registered their interest.
# ----------------------------------------------------------------------
itcl::body Rappture::DispatchObj::event {args} {
    set when "-now"
    set first [lindex $args 0]
    if {[string index $first 0] == "-"} {
        switch -- $first {
            -now   {
                set when "-now"
                set args [lrange $args 1 end]
            }
            -later {
                set when 1
                set args [lrange $args 1 end]
            }
            -idle  {
                set when "-idle"
                set args [lrange $args 1 end]
            }
            -after {
                set when [lindex $args 1]
                if {![string is int $when]} {
                    error "bad value \"$when\": should be int (time in ms)"
                }
                set args [lrange $args 2 end]
            }
            default {
                error "bad option \"$first\": should be -now, -later, -idle, or -after"
            }
        }
    }

    if {[llength $args] < 1} {
        error "wrong # args: should be \"event ?switches? !event ?args...?\""
    }
    set event [lindex $args 0]
    set args [lrange $args 1 end]
    if {![info exists _event2clients($event)]} {
        error "bad event \"$event\": should be [join [lsort [array names _event2clients]] {, }]"
    }

    switch -- $when {
        -now {
            _send $event all $args
        }
        -idle {
            set _extraargs($event) $args
            after cancel [itcl::code $this _send $event all @extra]
            after idle [itcl::code $this _send $event all @extra]
        }
        default {
            set _extraargs($event) $args
            after cancel [itcl::code $this _send $event all @extra]
            after $when [itcl::code $this _send $event all @extra]
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: cancel ?!event ...?
#        cancel all
#
# Used to cancel any event notifications pending for the specified
# list of events.  Notifications may be pending if a particular
# event was raised with the -idle or -after flags.
# ----------------------------------------------------------------------
itcl::body Rappture::DispatchObj::cancel {args} {
    if { $args == "all" } {
        foreach event [array names _event2clients] {
            after cancel [itcl::code $this _send $event all @extra]
        }
        return
    }
    foreach event $args {
        if {![info exists _event2clients($event)]} {
            error "bad event \"$event\": should be [join [lsort [array names _event2clients]] {, }]"
        }
        after cancel [itcl::code $this _send $event all @extra]
    }
}

# ----------------------------------------------------------------------
# USAGE: ispending !event
#
# Returns 1 if the specified !event is pending, and 0 otherwise.
# Notifications may be pending if a particular event was raised
# with the -idle or -after flags.
# ----------------------------------------------------------------------
itcl::body Rappture::DispatchObj::ispending {event} {
    set cmd [itcl::code $this _send $event all @extra]
    foreach id [after info] {
        set cmd2 [lindex [after info $id] 0]
        if {[string equal $cmd $cmd2]} {
            return 1
        }
    }
    return 0
}

# ----------------------------------------------------------------------
# USAGE: _send !event caller ?arglist?
#
# Used internally to send an event to one or all callers.  The event
# is sent by first invoking a callback to generate any data associated
# with the event.  This data, along with any extra args passed in,
# are added to the callback for the caller.
# ----------------------------------------------------------------------
itcl::body Rappture::DispatchObj::_send {event caller {arglist ""}} {
    if {$caller == "all"} {
        set caller $_event2clients($event)
    }
    if {$arglist == "@extra" && [info exists _extraargs($event)]} {
        set arglist $_extraargs($event)
    }

    # if there are any clients for this event, get the arguments ready
    set any 0
    foreach cname $caller {
        if {[info exists _dispatch($cname-$event)]} {
            set any 1
            break
        }
    }

    set dargs ""
    if {$any && [string length $_event2datacb($event)] > 0} {
        # get the args from the data callback
        set status [catch {
            uplevel #0 $_event2datacb($event) $event
        } dargs]

        if {$status != 0} {
            # anything go wrong? then throw a background error
            bgerror "$dargs\n(while dispatching $event)"
            set dargs ""
        }
    }
    # add any arguments added from command line
    eval lappend dargs $arglist

    foreach cname $caller {
        if {[info exists _dispatch($cname-$event)]} {
            set status [catch {
                uplevel #0 $_dispatch($cname-$event) event $event $dargs
            } result]

            if {$status != 0} {
                # anything go wrong? then throw a background error
                bgerror "$result\n(while dispatching $event to $cname)"
            }
        }
    }
}

# ----------------------------------------------------------------------
# BASE CLASS: Dispatcher
# ----------------------------------------------------------------------
itcl::class Rappture::Dispatcher {
    constructor {args} {
        Rappture::dispatcher _dispatcher
        $_dispatcher register !destroy

        eval configure $args
    }

    destructor {
        event !destroy
    }

    public method dispatch {args} {
        eval $_dispatcher dispatch $args
    }

    protected method register {args} {
        eval $_dispatcher register $args
    }
    protected method event {args} {
        eval $_dispatcher event $args object $this
    }

    private variable _dispatcher ""  ;# dispatcher for events
}
