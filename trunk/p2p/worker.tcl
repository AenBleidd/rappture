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
#  Copyright (c) 2008  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Rappture

# recognize other library files in this same directory
set dir [file dirname [info script]]
lappend auto_path $dir

# handle log file for this worker
log channel debug on
log channel system on

# set up a server at the first open port above 9101
set server [Server ::#auto 9101? -servername worker \
    -onprotocol worker_peers_protocol]

# ======================================================================
#  OPTIONS
#  These options get the worker started, but new option settings
#  some from the authority after this worker first connects.
# ======================================================================
set authority ""     ;# file handle for authority connection
set myaddress "?:?"  ;# address/port for this worker

# set of connections for authority servers
set options(authority_hosts) 127.0.0.1:9001

# register with the central authority at this frequency
set options(time_between_registers) 10000

# this worker should try to connect with this many other peers
set options(max_peer_connections) 4

# number of seconds between each check of system load
set options(time_between_load_checks) 60000

# ======================================================================
#  PEERS
# ======================================================================
# eventually set to the list of all peers in the network
set peers(all) ""

# list of peers that we're testing connections with
set peers(testing) ""

# ======================================================================
#  SYSTEM LOAD
# ======================================================================
# Check system load every so often.  When the load changes
# significantly, execute a "perftest" run to compute the current
# number of wonks available to this worker.

set sysload(lastLoad) -1
set sysload(wonks) -1
set sysload(measured) -1

after idle worker_load_check

# ======================================================================
#  PROTOCOL: hubzero:peer/1
#  This protocol gets used when another worker connects to this one.
# ======================================================================
$server protocol hubzero:peer/1

$server define hubzero:peer/1 exception {message} {
    log system "ERROR: $message"
}

# ----------------------------------------------------------------------
#  DIRECTIVE: identity
#  Used for debugging, so that each client can identify itself by
#  name to this worker.
# ----------------------------------------------------------------------
$server define hubzero:peer/1 identity {name} {
    variable cid
    variable handler
    $handler connectionName $cid $name
    return ""
}

# ----------------------------------------------------------------------
#  DIRECTIVE: ping
# ----------------------------------------------------------------------
$server define hubzero:peer/1 ping {} {
    return "pong"
}

# ======================================================================
#  PROTOCOL: hubzero:worker/1
#  The authority_connect procedure connects this worker to an authority
#  and begins the authority/worker protocol.  The directives below
#  handle the incoming (authority->worker) traffic.
# ======================================================================
proc authority_connect {} {
    global authority options
    log system "connecting to authorities..."

    authority_disconnect

    # scan through list of authorities and try to connect
    foreach addr [randomize $options(authority_hosts)] {
        if {[catch {Client ::#auto $addr} result] == 0} {
            set authority $result
            break
        }
    }

    if {"" == $authority} {
        error "can't connect to any known authorities"
    }

    # add handlers to process the incoming commands...
    $authority protocol hubzero:worker/1
    $authority define hubzero:worker/1 exception {args} {
        log system "error from authority: $args"
    }

    # ------------------------------------------------------------------
    #  DIRECTIVE: options <key1> <value1> <key2> <value2> ...
    #  These option settings coming from the authority override the
    #  option settings built into the client at the top of this
    #  script.  The authority probably has a more up-to-date list
    #  of other authorities and better policies for living within
    #  the network.
    # ------------------------------------------------------------------
    $authority define hubzero:worker/1 options {args} {
        global options server myaddress

        array set options $args

        if {[info exists options(ip)]} {
            set myaddress $options(ip):[$server port]
            log debug "my address $myaddress"
        }
        return ""
    }

    # ------------------------------------------------------------------
    #  DIRECTIVE: peers <listOfAddresses>
    #  This message comes in after this worker has sent the "peers"
    #  message to request the current list of peers.  The
    #  <listOfAddresses> is a list of host:port addresses that this
    #  worker should contact to enter the p2p network.
    # ------------------------------------------------------------------
    $authority define hubzero:worker/1 peers {plist} {
        global peers
        set peers(all) $plist
        after idle worker_peers_update

        # now that we've gotten the peers, we're done with the authority
        authority_disconnect
    }

    # ------------------------------------------------------------------
    #  DIRECTIVE: identity
    #  Used for debugging, so that each client can identify itself by
    #  name to this worker.
    # ------------------------------------------------------------------
    $authority define hubzero:worker/1 identity {name} {
        variable cid
        variable handler
        $handler connectionName $cid $name
        return ""
    }

    $authority send "protocol hubzero:worker/1"
    return $authority
}

proc authority_disconnect {} {
    global authority
    if {"" != $authority} {
        catch {itcl::delete object $authority}
        set authority ""
    }
}

# ======================================================================
#  USEFUL ROUTINES
# ======================================================================
# ----------------------------------------------------------------------
#  COMMAND:  worker_register
#
#  Invoked when this worker first starts up and periodically thereafter
#  to register the worker with the central authority, and to request
#  a list of peers that this peer can talk to.
# ----------------------------------------------------------------------
proc worker_register {} {
    global options server

    # connect to the authority and request a list of peers
    if {[catch {
        set client [authority_connect]
        $client send "listening [$server port]"
        $client send "peers"
    } result]} {
        log system "ERROR: $result"
    }

    # register again at regular intervals in case the authority
    # gets restarted in between.
    after $options(time_between_registers) worker_register
}

# ----------------------------------------------------------------------
#  COMMAND:  worker_load_check
#
#  Invoked when this worker first starts up and periodically thereafter
#  to compute the system load available to the worker.  If the system
#  load has changed significantly, then the "perftest" program is
#  executed to get a measure of performance available.  This program
#  returns a number of "wonks" which we report to peers as our available
#  performance.
# ----------------------------------------------------------------------
proc worker_load_check {} {
    global options sysload dir

    # see if the load has changed significantly
    set changed 0
    if {$sysload(lastLoad) < 0} {
        set changed 1
    } else {
        set load [Rappture::sysinfo load5]
        if {$load < 0.9*$sysload(lastLoad) || $load > 1.1*$sysload(lastLoad)} {
            set changed 1
        }
    }

    if {$changed} {
        set sysload(lastLoad) [Rappture::sysinfo load5]
puts "LOAD CHANGED: $sysload(lastLoad) [Rappture::sysinfo freeram freeswap]"
        set sysload(measured) [clock seconds]

        # Run the program, but don't use exec, since it blocks.
        # Instead, run it in the background and harvest its output later.
        set sysload(test) [open "| [file join $dir perftest]" r]
        fileevent $sysload(test) readable worker_load_results
    }

    # monitor the system load at regular intervals
    after $options(time_between_load_checks) worker_load_check
}

# ----------------------------------------------------------------------
#  COMMAND:  worker_load_results
#
#  Invoked automatically when the "perftest" run finishes.  Reads the
#  number of wonks reported on standard output and reports them to
#  other peers.
# ----------------------------------------------------------------------
proc worker_load_results {} {
    global sysload

    if {[catch {read $sysload(test)} msg] == 0} {
        if {[regexp {[0-9]+} $msg match]} {
puts "WONKS: $match"
            set sysload(wonks) $match
        } else {
            log system "ERROR: performance test failed: $msg"
        }
    }
    catch {close $sysload(test)}
    set sysload(test) ""
}

# ----------------------------------------------------------------------
#  COMMAND:  worker_peers_update
#
#  Invoked when this worker has received a list of peers, and from
#  time to time thereafter, to establish connections with other peers
#  in the network.  This worker picks a number of peers randomly from
#  the list of all available peers, and then pings each of them.
#  Timing results from the pings are stored away, and when all pings
#  have completed, this worker picks the best few connections and
#  keeps those.
# ----------------------------------------------------------------------
proc worker_peers_update {} {
    global options peers myaddress
    worker_peers_cleanup

    #
    # Pick a random group of peers and try to connect to them.
    # Start with the existing peers, and then add a random
    # bunch of others.
    #
    foreach key [array names peers current-*] {
        set peer $peers($key)
        lappend peers(testing) $peer
    }

    #
    # Pick other peers at random.  We don't have to try all
    # peers--just some number that is much larger than the
    # final number we want to talk to.
    #
    set ntest [expr {10 * $options(max_peer_connections)}]
    foreach addr [randomize $peers(all) $ntest] {
        if {$addr == $myaddress} {
            continue
        }
        if {![info exists peers(current-$addr)]
              && [catch {Client ::#auto $addr} peer] == 0} {
            # open a new connection to this address
            lappend peers(testing) $peer
            $peer protocol hubzero:peer/1

            $peer define hubzero:peer/1 exception {message} {
                log system "ERROR: $message"
            }

            $peer define hubzero:peer/1 identity {name} {
                variable cid
                variable handler
                $handler connectionName $cid $name
                return ""
            }

            # when we get a "pong" back, store the latency
            $peer define hubzero:peer/1 pong {} {
                global peers
                variable handler
puts "  pong from $handler"
                set now [clock clicks -milliseconds]
                set delay [expr {$now - $peers(ping-$handler-start)}]
                set peers(ping-$handler-latency) $delay
                incr peers(responses)
                worker_peers_finalize
                return ""
            }

            # start the ping/pong session with the peer
            $peer send "protocol hubzero:peer/1"

            # send tell this peer our name (for debugging)
            if {[log channel debug]} {
                $peer send "identity $myaddress"
            }
        }
    }

    #
    # Now, loop through all peers and send a "ping" message.
    #
    foreach peer $peers(testing) {
puts "pinging $peer = [$peer address]..."
        # mark the time and send a ping to this peer
        set peers(ping-$peer-start) [clock clicks -milliseconds]
        $peer send "ping"
    }

    # if this test takes too long, just give up
    after 10000 worker_peers_finalize -force
}

# ----------------------------------------------------------------------
#  COMMAND:  worker_peers_finalize ?-force?
#
#  Called after worker_peers_update has finished its business to
#  clean up all of the connections that were open for testing.
# ----------------------------------------------------------------------
proc worker_peers_finalize {{option -check}} {
    global peers options
    if {$option == "-force" || $peers(responses) == [llength $peers(testing)]} {
        # build a list:  {peer latency} {peer latency} ...
        set plist ""
        foreach obj $peers(testing) {
            if {[info exists peers(ping-$obj-latency)]} {
                lappend plist [list $obj $peers(ping-$obj-latency)]
            }
        }
puts "-------------------\nFINALIZING $::myaddress"
puts "PINGS: $plist"

        # sort the list and extract the top peers
        set plist [lsort -increasing -index 1 $plist]
        set plist [lrange $plist 0 [expr {$options(max_peer_connections)-1}]]
puts "TOP: $plist"

        set current [array names peers current-*]
puts "  current: $current"

        # transfer the top peers to the "current" list
        set all ""
        foreach rec $plist {
            set peer [lindex $rec 0]
            set addr [$peer address]
            set peers(current-$addr) $peer
            lappend all $addr

            # if it is already on the "current" list, then keep it
            set i [lsearch $current current-$addr]
            if {$i >= 0} {
                set current [lreplace $current $i $i]
            }
            set i [lsearch $peers(testing) $peer]
            if {$i >= 0} {
                set peers(testing) [lreplace $peers(testing) $i $i]
            }
        }
        log system "connected to peers: $all"
puts "  new current: [array names peers current-*]"
puts "  final: $all"
puts "  leftover: $current $peers(testing)"

        # get rid of old peers that we no longer want to talk to
        foreach leftover $current {
            itcl::delete object $peers($leftover)
            unset peers($leftover)
puts "  cleaned up $leftover (was on current list)"
        }

        # clean up after this test
        worker_peers_cleanup
    }
}

# ----------------------------------------------------------------------
#  COMMAND:  worker_peers_cleanup
#
#  Called after worker_peers_update has finished its business to
#  clean up all of the connections that were open for testing.
# ----------------------------------------------------------------------
proc worker_peers_cleanup {} {
    global peers
    foreach obj $peers(testing) {
        catch {itcl::delete object $obj}
puts "  cleaned up $obj (was on testing list)"
    }
    set peers(testing) ""

    foreach key [array names peers ping-*] {
        unset peers($key)
    }
    set peers(responses) 0

    after cancel worker_peers_finalize -force
}

# ----------------------------------------------------------------------
#  COMMAND:  worker_peers_protocol
#
#  Invoked whenever a peer sends their protocol message to this
#  worker.  Sends the same protocol name back, so the other worker
#  understands what protocol we're speaking.
# ----------------------------------------------------------------------
proc worker_peers_protocol {cid protocol} {
    if {"DEFAULT" != $protocol} {
        puts $cid [list protocol $protocol]
    }
}

# ======================================================================
#  Connect to one of the authorities to get a list of peers.  Then,
#  sit in the event loop and process events.
# ======================================================================
after idle worker_register
log system "starting..."
vwait main-loop
