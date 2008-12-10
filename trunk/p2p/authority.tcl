# ----------------------------------------------------------------------
#  P2P: central authority for all P2P activities
#
#  This server is the "superpeer" that all workers contact first
#  to join the network.  Clients and workers also contact the authority
#  to handle payments for services.
# ----------------------------------------------------------------------
#  Michael McLennan (mmclennan@purdue.edu)
# ======================================================================
#  Copyright (c) 2008  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================

# recognize other library files in this same directory
set dir [file dirname [info script]]
lappend auto_path $dir

# handle log file for this authority
log channel debug on
log channel system on

# ======================================================================
#  WORKER OPTIONS
# ======================================================================

# set of connections for authority servers
set worker_options(authority_hosts) \
    {127.0.0.1:9001 127.0.0.1:9002 127.0.0.1:9003}

# register with the central authority at this frequency
set worker_options(time_between_registers) 60000

# workers should try to connect with this many other peers
set worker_options(max_peer_connections) 4

# ======================================================================
# set up a server at the first open port above 9001
set server [Server ::#auto 9001? \
    -servername authority \
    -onconnect authority_client_address \
    -onprotocol authority_client_protocol \
]

# ----------------------------------------------------------------------
#  COMMAND: authority_client_address <cid> <addr> <port>
#
#  Invoked automatically whenever a client connects to the server.
#  Adds the client address/port to a list of all known clients.
#  Other clients searching for peers can ask for a list of all known
#  clients.
# ----------------------------------------------------------------------
proc authority_client_address {cid addr port} {
    global client2address
    set client2address($cid) $addr
}

# ----------------------------------------------------------------------
#  COMMAND: authority_client_protocol <cid> <protocol>
#
#  Invoked automatically whenever a client declares its protocol
#  to the server.  Clients speaking the "worker" protocol are
#  kept on a special list.
# ----------------------------------------------------------------------
proc authority_client_protocol {cid protocol} {
    global worker_options client2address
    if {"DEFAULT" != $protocol} {
        puts $cid [list protocol $protocol]
        puts $cid "options [array get worker_options] ip $client2address($cid)"
    }
}

# ======================================================================
#  PROTOCOL: hub:worker/1
# ======================================================================
$server protocol hubzero:worker/1

# ----------------------------------------------------------------------
#  DIRECTIVE: exception <message>
# ----------------------------------------------------------------------
$server define hubzero:worker/1 exception {args} {
    log system "error from worker: $args"
}

# ----------------------------------------------------------------------
#  DIRECTIVE: listening <port>
#  Workers use this to tell the authority what port they're listening
#  on.  Other workers should connect to this port.
# ----------------------------------------------------------------------
$server define hubzero:worker/1 listening {port} {
    global client2address workers
    variable handler
    variable cid

    # register this address on the list of workers
    set addr "$client2address($cid):$port"
    set workers($addr) "--"

    # name this connection for easier debugging
    $handler connectionName $cid $addr

    return ""
}

# ----------------------------------------------------------------------
#  DIRECTIVE: peers
# ----------------------------------------------------------------------
$server define hubzero:worker/1 peers {} {
    global workers
    return [list peers [array names workers]]
}

vwait main-loop
