# ----------------------------------------------------------------------
#  P2P: foreman node in P2P mesh, sending jobs out for execution
#
#  This file provides an API for clients to connect to the P2P network,
#  solicit bids on jobs, and send them off for execution.
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

# handle log file for this foreman
log channel error on
log channel debug on
log channel system on

proc ::bgerror {err} { log error "ERROR: $err $::errorInfo" }

# set of connections for authority servers
p2p::options register authority_hosts 127.0.0.1:9001

# ======================================================================
#  PROTOCOL: hubzero:foreman<-authority/1
#
#  The foreman initiates communication with the authority, and the
#  authority responds by sending the following messages back.
# ======================================================================
p2p::protocol::register hubzero:foreman<-authority/1 {
    # ------------------------------------------------------------------
    #  INCOMING: options <key1> <value1> <key2> <value2> ...
    #  These option settings coming from the authority override the
    #  option settings built into the client.  The authority probably
    #  has a more up-to-date list of other authorities and better
    #  policies for living within the network.
    # ------------------------------------------------------------------
    define options {args} {
        foreach {key val} $args {
            catch {p2p::options set $key $val}
        }
        return ""
    }

    # ------------------------------------------------------------------
    #  INCOMING: workers <listOfAddresses>
    #  This message comes in after this foreman has sent the "workers"
    #  message to request the current list of workers.  The
    #  <listOfAddresses> is a list of host:port addresses that this
    #  foreman should contact to enter the p2p network.
    # ------------------------------------------------------------------
    define workers {wlist} {
        global workers
        set workers $wlist
        foreman-solicit goto informed
        return ""
    }
}

# ======================================================================
#  PROTOCOL: hubzero:foreman<-worker/1
#
#  The foreman initiates communication with a worker, and the
#  worker responds by sending the following messages back.
# ======================================================================
p2p::protocol::register hubzero:foreman<-worker/1 {
    # ------------------------------------------------------------------
    #  INCOMING: proffer <token> <details>
    # ------------------------------------------------------------------
    define proffer {token details} {
puts "PROFFER:\n$details"
        after idle {foreman-solicit goto informed}
        return ""
    }
}

# ----------------------------------------------------------------------
#  PEER-TO-PEER CONNECTION
#
#  This is the state machine representing the connection of the foreman
#  to the peer-to-peer network.  We start in the "init" state, then
#  move to "fetching" while waiting for an authority to return a list
#  of workers, then move to "informed".  At any point, we can move
#  from "informed" to "connected" and connect to one of the workers
#  in the peer-to-peer network, then move back to "informed" when
#  the information has been gathered.
# ----------------------------------------------------------------------
StateMachine foreman-solicit
foreman-solicit statedata authoritycnx ""
foreman-solicit statedata workercnx ""

# sit here whenever we don't have a connection
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
foreman-solicit state init

# sit here while we're connected to the authority and fetching data
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
foreman-solicit state fetching

# when moving to the fetching state, connect to the authority
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
foreman-solicit transition init->fetching -onchange {
    # connect to the authority and request a list of peers
    statedata authoritycnx ""
    foreach addr [randomize [p2p::options get authority_hosts]] {
        if {[catch {p2p::client -address $addr \
            -sendprotocol hubzero:authority<-foreman/1 \
            -receiveprotocol hubzero:foreman<-authority/1} result] == 0} {
            statedata authoritycnx $result
            break
        }
    }

    if {"" != [statedata authoritycnx]} {
        [statedata authoritycnx] send "identity foreman"
        [statedata authoritycnx] send "workers"
    } else {
        error "can't connect to any authority\nAUTHORITY LIST: [p2p::options get authority_hosts]"
    }
}

# sit here after we've gotten a list of workers
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
foreman-solicit state informed

# when moving to the fetching state, connect to the authority
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
foreman-solicit transition fetching->informed -onchange {
    # connect to the authority and request a list of peers
    if {"" != [statedata authoritycnx]} {
        catch {itcl::delete object [statedata authoritycnx]}
        statedata authoritycnx ""
    }
    after idle {foreman-solicit goto connected}
}

# sit here when we're connected to the p2p network
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
foreman-solicit state connected

# when moving to the connected state, connect to the p2p worker
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
foreman-solicit transition informed->connected -onchange {
    # connect to the p2p network and send a solicitation
    global workers
    statedata workercnx ""
    foreach addr [randomize $workers 100] {
        if {[catch {p2p::client -address $addr \
            -sendprotocol hubzero:worker<-foreman/1 \
            -receiveprotocol hubzero:foreman<-worker/1} result] == 0} {
            statedata workercnx $result
            break
        }
    }

    if {"" != [statedata workercnx]} {
        [statedata workercnx] send "identity foreman"
        [statedata workercnx] send "solicit -job [expr {rand()}]"
    } else {
        after 2000 {foreman-solicit goto connected}
        error "can't connect to any worker in the p2p network"
    }
}

# when moving back to the informed state, disconnect from p2p network
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
foreman-solicit transition connected->informed -onchange {
    # have an open connection? then close it
    if {"" != [statedata workercnx]} {
        catch {itcl::delete object [statedata workercnx]}
        statedata workercnx ""
    }
}

# ----------------------------------------------------------------------
#  API
# ----------------------------------------------------------------------
namespace eval Rappture::foreman { # forward declaration }

# ----------------------------------------------------------------------
#  COMMAND:  Rappture::foreman::bids -callback <command>
#
#  Clients use this to solicit bids for jobs.  Connects to an
#  authority, if necessary, to determine a list of workers, and then
#  connects to one of the workers to kick off the bid process.
# ----------------------------------------------------------------------
proc Rappture::foreman::bids {args} {
    global workers

    if {![info exists workers] || [llength $workers] == 0} {
        foreman-solicit goto fetching
    } else {
        foreman-solicit goto connected
    }
}
