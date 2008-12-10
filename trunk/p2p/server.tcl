# ----------------------------------------------------------------------
#  LIBRARY: core server capability used for p2p infrastructure
# ----------------------------------------------------------------------
#  Michael McLennan (mmclennan@purdue.edu)
# ======================================================================
#  Copyright (c) 2008  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itcl

itcl::class Server {
    inherit Handler

    private variable _port  ;# stores the port that this server listens on

    # name for this server (for log messages)
    public variable servername "server"

    # this code fragment gets invoked with each new client
    public variable onconnect ""

    # this code fragment gets invoked when client declares the protocol
    public variable onprotocol ""

    constructor {port args} {
        #
        # Process option switches for the server.
        #
        eval configure $args

        #
        # Start up the server at the specified port.  If the port
        # number ends with a ?, then search for the first open port
        # above that.  The actual port can be queried later via the
        # "port" method.
        #
        if {[regexp {^[0-9]+$} $port]} {
            socket -server [itcl::code $this accept] $port
            set _port $port
        } elseif {[regexp {^[0-9]+\?$} $port]} {
            set pnum [string trimright $port ?]
            set tries 500
            while {[incr tries -1] > 0} {
                if {[catch {socket -server [itcl::code $this accept] $pnum} result]} {
                    incr pnum
                } else {
                    set _port $pnum
                    break
                }
            }
            if {$tries <= 0} {
                error "can't find an open port for server at $port"
            }
            log system "$servername started at port $_port"
        }
    }

    public method port {}
    public method connectionSpeaks {cid protocol}

    protected method handlerType {}

    private method accept {cid addr port}
}

# ----------------------------------------------------------------------
#  USAGE: port
#
#  Returns the port number that this server is listening on.  When
#  the server is first created, this can be set to a hard-coded value,
#  or to a value followed by a ?.  In that case, the server tries to
#  find the first open port.  The actual port is reported by this
#  method.
# ----------------------------------------------------------------------
itcl::body Server::port {} {
    return $_port
}

# ----------------------------------------------------------------------
#  USAGE: accept <cid> <addr> <port>
#
#  Invoked automatically whenever a client tries to connect to this
#  server.  The <cid> is the file handle for this client.  The <addr>
#  and <port> give the address and port number of the incoming client.
# ----------------------------------------------------------------------
itcl::body Server::accept {cid addr port} {
    fileevent $cid readable [itcl::code $this handle $cid]
    fconfigure $cid -buffering line
    connectionSpeaks $cid DEFAULT
    log system "accepted: $addr ($cid)"

    if {[string length $onconnect] > 0} {
        uplevel #0 [list $onconnect $cid $addr $port]
    }
}

# ----------------------------------------------------------------------
#  USAGE: connectionSpeaks <client> <protocol>
#
#  Used internally to define what protocol the entity on the other
#  side of the connection speaks.  This is usually invoked when that
#  entity sends the "protocol" message, and the built-in "protocol"
#  command in the DEFAULT parser uses this method to register the
#  protocol for the entity.
# ----------------------------------------------------------------------
itcl::body Server::connectionSpeaks {cid protocol} {
    chain $cid $protocol

    # if there's a callback for the protocol change, execute it here
    if {[string length $onprotocol] > 0} {
        uplevel #0 [list $onprotocol $cid $protocol]
    }
}

# ----------------------------------------------------------------------
#  USAGE: handlerType
#
#  Returns a descriptive string describing this handler.  Derived
#  classes override this method to provide their own string.  Used
#  for debug messages.
# ----------------------------------------------------------------------
itcl::body Server::handlerType {} {
    return "server"
}
