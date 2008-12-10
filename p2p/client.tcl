# ----------------------------------------------------------------------
#  LIBRARY: handles the client side of the connection in the p2p
#    infrastructure
# ----------------------------------------------------------------------
#  Michael McLennan (mmclennan@purdue.edu)
# ======================================================================
#  Copyright (c) 2008  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itcl

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

    protected method handlerType {}
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
            log debug "sending: $message"
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

# ----------------------------------------------------------------------
#  USAGE: handlerType
#
#  Returns a descriptive string describing this handler.  Derived
#  classes override this method to provide their own string.  Used
#  for debug messages.
# ----------------------------------------------------------------------
itcl::body Client::handlerType {} {
    return "client"
}
