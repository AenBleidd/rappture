# ----------------------------------------------------------------------
#  LIBRARY: handles the client side of the connection in the p2p
#    infrastructure
# ----------------------------------------------------------------------
#  Michael McLennan (mmclennan@purdue.edu)
# ======================================================================
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itcl

namespace eval p2p { # forward declaration }

# ======================================================================
#  USAGE: p2p::client ?-option value -option value ...?
#
#  Used to create a client connection to a peer-to-peer server.
#  Recognizes the following options:
#    -address ........... connect to server at this host:port
#    -sendprotocol ...... tell server we're speaking this protocol
#    -receiveprotocol ... server replies back with these commands
# ======================================================================
proc p2p::client {args} {
    array set options {
        -address ?
        -sendprotocol ""
        -receiveprotocol ""
    }
    foreach {key val} $args {
        if {![info exists options($key)]} {
            error "bad option \"$key\": should be [join [lsort [array names options]] {, }]"
        }
        set options($key) $val
    }

    # create the client
    set client [eval Client ::#auto $options(-address)]

    # install the protocol for incoming commands
    p2p::protocol::init $client $options(-receiveprotocol)

    # tell the server what protocol we'll be speaking
    $client send [list protocol $options(-sendprotocol)]

    return $client
}

# ======================================================================
#  CLASS: Client
# ======================================================================

itcl::class Client {
    inherit Handler

    private variable _addr "" ;# address that this client is connected to
    private variable _sid ""  ;# file handle for server connection

    constructor {addr args} {
        eval configure $args

        #
        # Connect to the server at the specified address, which is
        # specified as "host:port".
        #
        set alist [split $addr :]
        if {[llength $alist] != 2} {
            error "bad address \"$addr\": should be host:port"
        }
        foreach {host port} $alist break
        set _sid [socket $host $port]
        connectionName $_sid $host:$port
        connectionSpeaks $_sid DEFAULT

        fileevent $_sid readable [itcl::code $this handle $_sid]
        fconfigure $_sid -buffering line

        set _addr $addr
    }

    destructor {
        catch {close $_sid}
    }

    public method send {message}
    public method address {}
}

# ----------------------------------------------------------------------
#  USAGE: send <message>
#
#  Used to send a <message> off to the server.  If the connection
#  was unexpectedly closed, then this method does nothing.
# ----------------------------------------------------------------------
itcl::body Client::send {message} {
    if {"" != $_sid} {
        if {[eof $_sid]} {
            set _sid ""
        } else {
            log debug "outgoing message to [address]: $message"
            puts $_sid $message
        }
    }
}

# ----------------------------------------------------------------------
#  USAGE: address
#
#  Returns the address that this client is connected to.  This is
#  the host:port passed in when the client was created.
# ----------------------------------------------------------------------
itcl::body Client::address {} {
    return $_addr
}
