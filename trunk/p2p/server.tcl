# ----------------------------------------------------------------------
#  LIBRARY: core server capability used for p2p infrastructure
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
#  USAGE: p2p::server ?-option value -option value ...?
#
#  Used to create a new peer-to-peer server object for this program.
#  Recognizes the following options:
#    -port ........ port number that the server listens on
#    -protocols ... list of protocol root names that the server handles
#               ... any other option supported by Server class
# ======================================================================
proc p2p::server {args} {
    set port "?"
    set protocols ""
    set options ""
    foreach {key val} $args {
        switch -- $key {
            -port { set port $val }
            -protocols { set protocols $val }
            default { lappend options $key $val }
        }
    }

    if {[llength $protocols] == 0} {
        error "server needs at least one value for -protocols"
    }

    # create the server
    set server [eval Server ::#auto $port $options]

    # install the protocols that this server recognizes
    foreach name $protocols {
        p2p::protocol::init $server $name
    }
    return $server
}

# ======================================================================
#  CLASS: Server
# ======================================================================
itcl::class Server {
    inherit Handler

    private variable _port  ;# stores the port that this server listens on

    # name for this server (for log messages)
    public variable servername "server"

    # this code fragment gets invoked with each new client
    public variable onconnect ""

    # this code fragment gets invoked when client declares the protocol
    public variable onprotocol ""

    # this code fragment gets invoked when client drops
    public variable ondisconnect ""

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
    public method broadcast {args}
    public method connectionSpeaks {cid protocol}

    protected method dropped {cid}
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
#  USAGE: broadcast ?-protocol <name>? ?-avoid <avoidList>? <message>
#
#  Sends a <message> to all clients connected to this server.  If a
#  client address appears on the -avoid list, then that client is
#  avoided.  If the -protocol is specified, then the message is sent
#  only to clients who match the glob-style pattern for the protocol
#  name.
# ----------------------------------------------------------------------
itcl::body Server::broadcast {args} {
    set pattern "*"
    set avoidList ""
    set i 0
    while {$i < [llength $args]} {
        set option [lindex $args $i]
        if {[string index $option 0] == "-"} {
            switch -- $option {
                -protocol {
                    set pattern [lindex $args [expr {$i+1}]]
                    incr i 2
                }
                -avoid {
                    set avoidList [lindex $args [expr {$i+1}]]
                    incr i 2
                }
                -- {
                    incr i
                    break
                }
                default {
                    error "bad option \"$option\": should be -avoid, -protocol, or --"
                }
            }
        } else {
            break
        }
    }
    if {$i != [llength $args]-1} {
        error "wrong # args: should be \"broadcast ?-protocol pattern? ?-avoid clients? message\""
    }
    set message [lindex $args end]

    set nmesgs 0
    foreach cid [connections $pattern] {
        set addr [lindex [connectionName $cid] 0]  ;# x.x.x.x (sockN)
        if {[llength $avoidList] == 0 || [lsearch $avoidList $addr] < 0} {
puts "  inbound => [connectionName $cid]"
            if {[catch {puts $cid $message} result] == 0} {
                incr nmesgs
            } else {
                log error "ERROR: broadcast failed for $cid: $result"
                log error "  (message was $message)"
            }
        }
    }
    return $nmesgs
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
#  USAGE: dropped <cid>
#
#  Invoked automatically whenever a client connection drops, to
#  log the event and remove all trace of the client.  Invokes any
#  command hook for this server to note the fact that the client
#  has dropped.
# ----------------------------------------------------------------------
itcl::body Server::dropped {cid} {
    # if there's a callback to handle the drop, execute it here
    if {[string length $ondisconnect] > 0} {
        uplevel #0 [list $ondisconnect [connectionName $cid]]
    }

    # call the base class method to clean up after the client
    chain $cid
}
