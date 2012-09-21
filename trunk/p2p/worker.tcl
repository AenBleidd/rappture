# ----------------------------------------------------------------------
#  P2P: worker node in P2P mesh bidding on and executing jobs
#
#  This server is a typical worker node in the P2P mesh for HUBzero
#  job execution.  It talks to other workers to distribute the load
#  of job requests.  It bids on jobs based on its current resources
#  and executes jobs to earn points from the central authority.
# ----------------------------------------------------------------------
#  Michael McLennan (mmclennan@purdue.edu)
# ======================================================================
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Rappture

# recognize other library files in this same directory
set dir [file dirname [info script]]
lappend auto_path $dir

# handle log file for this worker
log channel error on
log channel system on
log channel debug on

proc ::bgerror {err} { log error "ERROR: $err $::errorInfo" }


set myaddress "?:?"  ;# address/port for this worker

# set of connections for authority servers
p2p::options register authority_hosts 127.0.0.1:9001

# register with the central authority at this frequency
p2p::options register time_between_authority_checks 60000

# rebuild the peer-to-peer network at this frequency
p2p::options register time_between_network_rebuilds 600000

# this worker should try to connect with this many other peers
p2p::options register max_peer_connections 4

# workers propagate messages until time-to-live reaches 0
p2p::options register peer_time_to_live 4

# ======================================================================
#  PROTOCOL: hubzero:worker<-authority/1
#
#  The worker initiates communication with the authority, and the
#  authority responds by sending these messages.
# ======================================================================
p2p::protocol::register hubzero:worker<-authority/1 {

    # ------------------------------------------------------------------
    #  INCOMING: options <key1> <value1> <key2> <value2> ...
    #  These option settings coming from the authority override the
    #  option settings built into the client.  The authority probably
    #  has a more up-to-date list of other authorities and better
    #  policies for living within the network.
    # ------------------------------------------------------------------
    define options {args} {
        global server myaddress

        array set options $args
        foreach key [array names options] {
            catch {p2p::options set $key $options($key)}
        }

        if {[info exists options(ip)]} {
            set myaddress $options(ip):[$server port]
            log debug "my address $myaddress"
        }
        return ""
    }

    # ------------------------------------------------------------------
    #  INCOMING: peers <listOfAddresses>
    #  This message comes in after this worker has sent the "peers"
    #  message to request the current list of peers.  The
    #  <listOfAddresses> is a list of host:port addresses that this
    #  worker should contact to enter the p2p network.
    # ------------------------------------------------------------------
    define peers {plist} {
        global peers
        set peers(all) $plist
        after idle {peer-network goto measure}

        # now that we've gotten the peers, we're done with the authority
        authority-connection goto idle
        return ""
    }

    # ------------------------------------------------------------------
    #  INCOMING: identity
    #  Used for debugging, so that each client can identify itself by
    #  name to this worker.
    # ------------------------------------------------------------------
    define identity {name} {
        variable cid
        variable handler
        $handler connectionName $cid $name
        return ""
    }
}

# ======================================================================
#  PROTOCOL: hubzero:worker<-foreman/1
#
#  The foreman initiates communication with a worker, and sends the
#  various messages supported below.
# ======================================================================
p2p::protocol::register hubzero:worker<-foreman/1 {

    # ------------------------------------------------------------------
    #  INCOMING: solicit
    #  Foremen send this message to solicit bids for a simulation job.
    # ------------------------------------------------------------------
    define solicit {args} {
        variable cid
        log debug "solicitation request from foreman: $args"
        eval Solicitation ::#auto $args -connection $cid
        return ""
    }
}

# ======================================================================
#  PROTOCOL: hubzero:workers<-workerc/1
#
#  Workers initiate connections with other workers as peers.  The
#  following messages are sent by worker clients to the worker server.
# ======================================================================
p2p::protocol::register hubzero:workers<-workerc/1 {
    # ------------------------------------------------------------------
    #  INCOMING: identity
    #  Used for debugging, so that each client can identify itself by
    #  name to this worker.
    # ------------------------------------------------------------------
    define identity {name} {
        variable cid
        variable handler
        $handler connectionName $cid $name
        return ""
    }

    # ------------------------------------------------------------------
    #  INCOMING: ping
    #  If another worker sends "ping", they are trying to measure the
    #  speed of the connection.  Send back a "pong" message.  If this
    #  worker is close by, the other worker will stay connected to it.
    # ------------------------------------------------------------------
    define ping {} {
        return "pong"
    }

    # ------------------------------------------------------------------
    #  INCOMING: solicit -job info -path hosts -token xxx
    #  Workers send this message on to their peers to solicit bids
    #  for a simulation job.
    # ------------------------------------------------------------------
    define solicit {args} {
        variable cid
        log debug "solicitation request from peer: $args"
        eval Solicitation ::#auto $args -connection $cid
        return ""
    }

    # ------------------------------------------------------------------
    #  INCOMING: proffer <token> <details>
    #  Workers send this message back after a "solicit" request with
    #  details about what they can offer in terms of CPU power.  When
    #  a worker has received all replies from its peers, it sends back
    #  its own proffer message to the client that started the
    #  solicitation.
    # ------------------------------------------------------------------
    define proffer {token details} {
        Solicitation::proffer $token $details
        return ""
    }
}

# ======================================================================
#  PROTOCOL: hubzero:workerc<-workers/1
#
#  The following messages are received by a worker client in response
#  to the requests that they send to a worker server.
# ======================================================================
p2p::protocol::register hubzero:workerc<-workers/1 {
    # ------------------------------------------------------------------
    #  INCOMING: identity
    #  Used for debugging, so that each client can identify itself by
    #  name to this worker.
    # ------------------------------------------------------------------
    define identity {name} {
        variable cid
        variable handler
        $handler connectionName $cid $name
        return ""
    }

    # ------------------------------------------------------------------
    #  INCOMING: pong
    #  When forming the peer-to-peer network, workers send ping/pong
    #  messages to one another to measure the latency.
    # ------------------------------------------------------------------
    define pong {} {
        global peers
        variable handler
        set now [clock clicks -milliseconds]
        set delay [expr {$now - $peers(ping-$handler-start)}]
        set peers(ping-$handler-latency) $delay
        if {[incr peers(responses)] >= [llength $peers(testing)]} {
            after cancel {peer-network goto finalize}
            after idle {peer-network goto finalize}
        }
        return ""
    }

    # ------------------------------------------------------------------
    #  INCOMING: solicit -job info -path hosts -token xxx
    #  Workers send this message on to their peers to solicit bids
    #  for a simulation job.
    # ------------------------------------------------------------------
    define solicit {args} {
        variable cid
        log debug "solicitation request from peer: $args"
        eval Solicitation ::#auto $args -connection $cid
        return ""
    }

    # ------------------------------------------------------------------
    #  INCOMING: proffer <token> <details>
    #  Workers send this message back after a "solicit" request with
    #  details about what they can offer in terms of CPU power.  When
    #  a worker has received all replies from its peers, it sends back
    #  its own proffer message to the client that started the
    #  solicitation.
    # ------------------------------------------------------------------
    define proffer {token details} {
        Solicitation::proffer $token $details
        return ""
    }
}

# ----------------------------------------------------------------------
#  COMMAND:  broadcast_to_peers <message> ?<avoidList>?
#
#  Used to broadcast a message out to all peers.  The <message> must
#  be one of the commands defined in the worker protocol.  If there
#  are no peer connections yet, this command does nothing.  If any
#  peer appears on the <avoidList>, it is skipped.  Returns the
#  number of messages sent out, so this peer knows how many replies
#  to wait for.
# ----------------------------------------------------------------------
proc broadcast_to_peers {message {avoidList ""}} {
    global server peers

    #
    # Build a list of all peer addresses, so we know who we're
    # going to send this message out to.
    #
    set recipients ""
    foreach key [array names peers current-*] {
        set addr [$peers($key) address]
        if {[lsearch $avoidList $addr] < 0} {
            lappend recipients $addr
        }
    }
    foreach cid [$server connections hubzero:workers<-workerc/1] {
        set addr [lindex [$server connectionName $cid] 0]  ;# x.x.x.x (sockN)
        if {[llength $avoidList] == 0 || [lsearch $avoidList $addr] < 0} {
            lappend recipients $addr
        }
    }

    #
    # If the message has any @RECIPIENTS fields, replace them
    # with the list of recipients
    #
    regsub -all @RECIPIENTS $message $recipients message

    #
    # Send the message out to all peers.  Keep a count and double-check
    # it against the list of recipients generated above.
    #
    set nmesgs 0

    # send off to other workers that this one has connected to
    foreach key [array names peers current-*] {
        set addr [$peers($key) address]
        if {[lsearch $avoidList $addr] < 0} {
            if {[catch {$peers($key) send $message} err] == 0} {
                incr nmesgs
            } else {
                log error "ERROR: broadcast failed to [$peers($key) address]: $result\n  (message was \"$message\")"
            }
        }
    }

    # send off to other workers that connected to this one
    foreach cid [$server connections hubzero:workers<-workerc/1] {
        set addr [lindex [$server connectionName $cid] 0]  ;# x.x.x.x (sockN)
        if {[llength $avoidList] == 0 || [lsearch $avoidList $addr] < 0} {
            if {[catch {puts $cid $message} result] == 0} {
                incr nmesgs
            } else {
                log error "ERROR: broadcast failed for $cid: $result"
                log error "  (message was $message)"
            }
        }
    }

    # did we send the right number of messages?
    if {[llength $recipients] != $nmesgs} {
        log error "ERROR: sent only $nmesgs messages to peers {$recipients}"
    }

    return $nmesgs
}

# ----------------------------------------------------------------------
#  COMMAND:  worker_got_protocol
#
#  Invoked whenever a peer sends their protocol message to this
#  worker.  Sends the same protocol name back, so the other worker
#  understands what protocol we're speaking.
# ----------------------------------------------------------------------
proc worker_got_protocol {cid protocol} {
    switch -glob -- $protocol {
        *<-worker* {
            puts $cid [list protocol hubzero:workerc<-workers/1]
        }
        *<-foreman* {
            puts $cid [list protocol hubzero:foreman<-worker/1]
        }
        DEF* {
            # do nothing
        }
        default {
            error "don't recognize protocol \"$protocol\""
        }
    }
}

# ----------------------------------------------------------------------
#  COMMAND:  worker_got_dropped <address>
#
#  Invoked whenever an inbound connection to this worker drops.
#  At some point we should try updating our connections to other
#  peers, to replace this missing connection.
# ----------------------------------------------------------------------
proc worker_got_dropped {addr} {
    global peers
    if {[info exists peers(current-$addr)]} {
        unset peers(current-$addr)
        after cancel {peer-network goto measure}
        after 5000 {peer-network goto measure}
log debug "peer dropped: $addr"
    }
}

# ======================================================================
#  Connect to one of the authorities to get a list of peers.  Then,
#  sit in the event loop and process events.
# ======================================================================
p2p::wonks::init

set server [p2p::server -port 9101? \
        -protocols {hubzero:workers<-workerc hubzero:worker<-foreman} \
        -servername worker \
        -onprotocol worker_got_protocol \
        -ondisconnect worker_got_dropped]

# ----------------------------------------------------------------------
#  AUTHORITY CONNECTION
#
#  This is the state machine representing the connection to the
#  authority server.  We start in the "idle" state.  Whenever we
#  try to move to "connected", we'll open a connection to an authority
#  and request updated information on workers.  If that connection
#  fails, we'll get kicked back to "idle" and we'll try again later.
# ----------------------------------------------------------------------
StateMachine authority-connection
authority-connection statedata cnx ""

# sit here whenever we don't have a connection
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
authority-connection state idle -onenter {
    # have an open connection? then close it
    if {"" != [statedata cnx]} {
        catch {itcl::delete object [statedata cnx]}
        statedata cnx ""
    }
    # try to connect again later
    set delay [p2p::options get time_between_authority_checks]
    after $delay {authority-connection goto connected}
} -onleave {
    after cancel {authority-connection goto connected}
}

# sit here after we're connected and waiting for a response
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
authority-connection state connected

# when moving to the connected state, make a connection
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
authority-connection transition idle->connected -onchange {
    global server

    # connect to the authority and request a list of peers
    statedata cnx ""
    foreach addr [randomize [p2p::options get authority_hosts]] {
        if {[catch {p2p::client -address $addr \
            -sendprotocol hubzero:authority<-worker/1 \
            -receiveprotocol hubzero:worker<-authority/1} result] == 0} {
            statedata cnx $result
            break
        }
    }

    if {"" != [statedata cnx]} {
        [statedata cnx] send "listening [$server port]"
        [statedata cnx] send "peers"
    } else {
        error "can't connect to any authority\nAUTHORITY LIST: [p2p::options get authority_hosts]"
    }
}

# ----------------------------------------------------------------------
#  PEER-TO-PEER CONNECTIONS
#
#  This is the state machine representing the network of worker peers.
#  We start in the "idle" state.  From time to time we move to the
#  "measure" state and attempt to establish connections with a set
#  peers.  We then wait for ping/pong responses and move to "finalize".
#  When we have all responses, we move to "finalize" and finalize all
#  connections, and then move back to "idle".
# ----------------------------------------------------------------------
StateMachine peer-network
peer-network statedata cnx ""

# sit here when the peer network is okay
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
peer-network state idle -onenter {
    # try to connect again later
    set delay [p2p::options get time_between_network_rebuilds]
    after $delay {peer-network goto measure}
} -onleave {
    after cancel {peer-network goto measure}
}

# sit here when we need to rebuild the peer-to-peer network
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
peer-network state measure

# when moving to the start state, make connections to a bunch of peers
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
peer-network transition idle->measure -onchange {
    global server myaddress peers

    #
    # Get a list of workers already connected to this one
    # with an inbound connection to the server.
    #
    foreach cid [$server connections hubzero:workers<-workerc/1] {
        set addr [lindex [$server connectionName $cid] 0]
        set inbound($addr) $cid
    }

    #
    # Pick a random group of peers and try to connect to them.
    # Start with the existing peers to see if their connection
    # is still favorable, and then add a random bunch of others.
    #
    set peers(testing) ""
    set peers(responses) 0
    foreach key [array names peers current-*] {
        set peer $peers($key)
        lappend peers(testing) $peer
    }

    #
    # Pick other peers at random.  We don't have to try all
    # peers--just some number that is much larger than the
    # final number we want to talk to.
    #
    set maxpeers [p2p::options get max_peer_connections]
    foreach addr [randomize $peers(all) [expr {10*$maxpeers}]] {
        #
        # Avoid connecting to ourself, or to peers that we're
        # already connected to (either as inbound connections
        # to the server or as outbound connections to others).
        #
        if {$addr == $myaddress
              || [info exists peers(current-$addr)]
              || [info exists inbound($addr)]} {
            continue
        }

        if {[catch {p2p::client -address $addr \
            -sendprotocol hubzero:workers<-workerc/1 \
            -receiveprotocol hubzero:workerc<-workers/1} cnx]} {
            continue
        }
        $cnx send "identity $myaddress"
        lappend peers(testing) $cnx

        # have enough connections to test?
        if {[llength $peers(testing)] >= 2*$maxpeers} {
            break
        }
    }

    #
    # Now, loop through all peers and send a "ping" message.
    #
    foreach cnx $peers(testing) {
        # mark the time and send a ping to this peer
        set peers(ping-$cnx-start) [clock clicks -milliseconds]
        $cnx send "ping"
    }

    # if this test takes too long, just give up
    after 10000 {peer-network goto finalize}
}

# sit here when we need to rebuild the peer-to-peer network
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
peer-network state finalize -onenter {
    after cancel {peer-network goto finalize}

    # everything is finalized now, so go to idle
    after idle {peer-network goto idle}
}

# when moving to the finalize state, decide on the network of peers
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
peer-network transition measure->finalize -onchange {
    global peers

    set maxpeers [p2p::options get max_peer_connections]

    # build a list:  {peer latency} {peer latency} ...
    set plist ""
    foreach obj $peers(testing) {
        if {[info exists peers(ping-$obj-latency)]} {
            lappend plist [list $obj $peers(ping-$obj-latency)]
        }
    }

    # sort the list and extract the top peers
    set plist [lsort -increasing -index 1 $plist]
    set plist [lrange $plist 0 [expr {$maxpeers-1}]]

    set currentpeers [array names peers current-*]

    # transfer the top peers to the "current" list
    set all ""
    foreach rec $plist {
        set peer [lindex $rec 0]
        set addr [$peer address]
        set peers(current-$addr) $peer
        lappend all $addr

        # if it is already on the "current" list, then keep it
        set i [lsearch $currentpeers current-$addr]
        if {$i >= 0} {
            set currentpeers [lreplace $currentpeers $i $i]
        }
        set i [lsearch $peers(testing) $peer]
        if {$i >= 0} {
            set peers(testing) [lreplace $peers(testing) $i $i]
        }
    }
    log system "connected to peers: $all"

    # get rid of old peers that we no longer want to talk to
    foreach leftover $currentpeers {
        itcl::delete object $peers($leftover)
        unset peers($leftover)
    }

    # clean up after this test
    foreach obj $peers(testing) {
        catch {itcl::delete object $obj}
    }
    set peers(testing) ""

    foreach key [array names peers ping-*] {
        unset peers($key)
    }
    set peers(responses) 0
}

# ----------------------------------------------------------------------
#  JOB SOLICITATION
#
#  Each worker can receive a "solicit" request, asking for information
#  about performance and price of peers available to work.  Each worker
#  sends the request on to its peers, then gathers the information
#  and sends it back to the client or to the peer requesting the
#  information.  There can be multiple requests going on at once,
#  and each may have different job types and return different info,
#  so the class below helps to watch over each request until it has
#  completed.
# ----------------------------------------------------------------------
itcl::class Solicitation {
    public variable connection ""
    public variable job ""
    public variable path ""
    public variable avoid ""
    public variable token ""

    private variable _serial ""
    private variable _response ""
    private variable _waitfor 0
    private variable _timeout ""

    constructor {args} {
        eval configure $args

        global myaddress
        lappend path $myaddress
        lappend avoid $myaddress
        set _serial [incr counter]
        set all($_serial) $this

        set delay "idle"  ;# finalize after waiting for responses
        set ttl [p2p::options get peer_time_to_live]
        if {[llength $path] < $ttl} {
            set mesg [list solicit -job $job -path $path -avoid "$avoid @RECIPIENTS" -token $_serial]
            set _waitfor [broadcast_to_peers $mesg $avoid]

            if {$_waitfor > 0} {
                # add a delay proportional to ttl + time for wonks measurement
                set delay [expr {($ttl-[llength $path]-1)*1000 + 3000}]
            }
        }
        set _timeout [after $delay [itcl::code $this finalize]]
    }
    destructor {
        after cancel $_timeout
        catch {unset all($_serial)}
    }

    # this adds the info from each proffer to this solicitation
    method response {details} {
        set addr [lindex $details 0]
        append _response $details "\n"
        if {[incr _waitfor -1] <= 0} {
            finalize
        }
    }

    # called to finalize when all peers have responded back
    method finalize {} {
        global myaddress

        # filter out duplicate info from clients
        set block ""
        foreach line [split $_response \n] {
            set addr [lindex $line 0]
            if {"" != $addr && ![info exists response($addr)]} {
                append block $line "\n"
                set response($addr) 1
            }
        }
        # add details about this client to the message
        append block "$myaddress -job $job -cost 1 -wonks [p2p::wonks::current]"

        # send the composite results back to the caller
        set cmd [list proffer $token $block]
        if {[catch {puts $connection $cmd} err]} {
            log error "ERROR while sending back proffer: $err"
        }
        itcl::delete object $this
    }

    proc proffer {serial details} {
        if {[info exists all($serial)]} {
            $all($serial) response $details
        }
    }

    common counter 0 ;# generates serial nums for objects
    common all       ;# maps serial num => solicitation object
}

# ----------------------------------------------------------------------
log system "starting..."

after idle {
    authority-connection goto connected
}

vwait main-loop
