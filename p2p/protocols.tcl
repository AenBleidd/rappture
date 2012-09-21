# ----------------------------------------------------------------------
#  P2P: protocol initialization and registration
#
#  This file contains p2p::init and p2p::connect routines used to
#  add protocols to Client and Server objects in the p2p system.
#  New protocols are stored in the various p-*.tcl files.  This
#  code loads those files, obtains a list of known protocols, and
#  then registers the latest version of a protocol in a given
#  Client or Server connection.
# ----------------------------------------------------------------------
#  Michael McLennan (mmclennan@purdue.edu)
# ======================================================================
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================

namespace eval p2p { # forward declaration }

namespace eval p2p::protocol {
    variable name2defn  ;# maps protocol name => body of code defining it
    variable version    ;# maps protocol name => highest /version number
    variable _tmpobj    ;# used during init/define calls
    variable _tmpproto  ;# used during init/define calls
}

# ======================================================================
#  USAGE: p2p::protocol::register <name> <code>
#
#  Various p-*.tcl files use this to define a particular protocol
#  called <name>.  Calling p2p::protocol::init adds that protocol to
#  a particular Handler object.  This is done by executing the <code>,
#  which invokes a series of "define" operations to define the
#  protocol handlers.
# ======================================================================
proc p2p::protocol::register {name code} {
    variable name2defn
    variable version

    # store the code needed for this version for later call to init
    if {[info exists name2defn($name)]} {
        error "protocol \"$name\" already exists"
    }
    set name2defn($name) $code

    # extract the version number from the protocol and save the highest num
    if {![regexp {(.+)/([0-9]+)$} $name match root vnum]} {
        set root $name
        set vnum 1
    }
    if {![info exists version($root)] || $vnum > $version($root)} {
        set version($root) $vnum
    }
}

# ======================================================================
#  USAGE: p2p::protocol::init <handlerObj> <protocol>
#
#  Adds all protocol versions matching the <protocol> root name to
#  the given <handlerObj> object.  For example, the <protocol> might
#  be "a->b", and this routine will add "a->b/1", "a->b/2", etc.
#  Returns the highest version of the protocol added to the object.
# ======================================================================
proc p2p::protocol::init {obj protocol} {
    variable name2defn
    variable version
    variable _tmpobj
    variable _tmpproto

    # trim off any version number included in the protocol name
    regexp {(.+)/[0-9]+$} $protocol match protocol

    set plist $protocol
    eval lappend plist [array names name2defn $protocol/*]
    set _tmpobj $obj  ;# used via "define" statements in "catch" below

    set added 0
    foreach name $plist {
        if {[info exists name2defn($name)]} {
            $obj protocol $name
            $obj define $name exception {message} {
                variable cid
                variable handler
                log error "ERROR from client [$handler connectionName $cid]:  $message"
            }
            $obj define $name identity {name} {
                variable cid
                variable handler
                $handler connectionName $cid $name
                return ""
            }

            # define the rest of the protocol as registered earlier
            set _tmpproto $name  ;# used via "define" statements below
            if {[catch $name2defn($name) err]} {
                error $err "$err\n    (while adding protocol $name to $obj)"
            }
            incr added
        }
    }

    if {$added == 0} {
        error "can't find protocol definition matching \"$protocol\""
    }
    return $version($protocol)
}

# ======================================================================
#  USAGE: p2p::protocol::define <name> <args> <body>
#
#  This gets called implicitly by the bodies of code invoked in the
#  p2p::protocol::init statements above.  Code bodies should contain
#  a series of "define" statements, and this is the "define" that
#  gets invoked, which passes the call onto the _tmpobj where the
#  protocol is being installed.
# ======================================================================
proc p2p::protocol::define {args} {
    variable _tmpobj
    variable _tmpproto
    eval $_tmpobj define $_tmpproto $args
}
