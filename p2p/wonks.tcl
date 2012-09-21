# ----------------------------------------------------------------------
#  P2P: wonks
#
#  This code manages the calculation of "wonks" within a worker node.
#  Wonks are measured by executing the "perftest" program on the
#  worker node to see how fast it can finish the calculation.
# ----------------------------------------------------------------------
#  Michael McLennan (mmclennan@purdue.edu)
# ======================================================================
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Rappture

namespace eval p2p { # forward declaration }

namespace eval p2p::wonks {
    # remember this directory -- perftest sits here too
    variable dir [file dirname [info script]]

    # version info from perftest
    variable version "?"
}

# number of seconds between each check of system load
p2p::options register wonks_time_between_checks 60000

# fraction between 0-1 indicating a significant change in wonks
p2p::options register wonk_tolerance 0.1

# ----------------------------------------------------------------------
#  WONKS CHECK
#
#  This is the state machine controlling the periodic testing for
#  the available wonks on this machine.  The "perftest" program is
#  executed from time to time to compute the available wonks, and
#  the results are stored in an array of tuples.  At any point, we
#  can estimate the number of wonks by getting the load and available
#  memory and interpolating within the available tuple data.
# ----------------------------------------------------------------------
StateMachine wonks-check

# sit here at times between sampling
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
wonks-check state idle -onenter {
    set delay [p2p::options get wonks_time_between_checks]
    after $delay {wonks-check goto checking}
} -onleave {
    after cancel {wonks-check goto checking}
}

# sit here while we're sampling
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
wonks-check state checking

# when moving to the checking state, test available wonks
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
wonks-check transition idle->checking -onchange {
    set data [wonks-check statedata perftable]

    set getWonks 0  ;# probably won't have to use perftest

    # measure system load and estimate the available wonks
    foreach {load mem} [Rappture::sysinfo load1 freeswap] break
    set load [format "%.1f" $load]
    set mem [expr {$mem/1048576}]  ;# in megabytes
    set wval 0
    set nsamples 0

    if {![$data inrange [list $load $mem -- --]]} {
        # outside range of data -- must call perftest
        set getWonks 1
    } else {
        set rec [$data find -nearest [list $load $mem -- --]]
        set prevload [lindex $rec 0]
        set prevmem [lindex $rec 1]
        set tol [p2p::options get wonk_tolerance]

        set closeEnough 1
        if {"" == $prevload || ($load+$prevload > 0
              && abs($load-$prevload)/(0.5*double($load+$prevload)) > $tol)} {
            # load differs by more than the tolerance -- must test
            set closeEnough 0
        } elseif {"" == $prevmem || ($mem+$prevmem > 0
              && abs($mem-$prevmem)/(0.5*double($mem+$prevmem)) > $tol)} {
            set closeEnough 0
        }

        if {$closeEnough} {
            # close enough to some other sample point
            # update the existing data point in the table
            set load $prevload
            set mem $prevmem

            set wval [lindex $rec 2]
            set nsamples [lindex $rec 3]
            if {$nsamples < 5} {
                # not very many samples? then do another
                set getWonks 1
            } elseif {$nsamples < 50 && rand() > 0.9} {
                # if we have a lot of samples do another from time to time
                set getWonks 1
            }
        }
    }

    # if we need a new reading, exec the perftest program...
    if {$getWonks} {
        # Run the program, but don't use exec, since it blocks.
        # Instead, run it in the background and harvest its output later.
        set prog [open "| [file join $p2p::wonks::dir perftest]" r]
        fileevent $prog readable {wonks-check goto update}
        wonks-check statedata sysinfo [list $load $mem $wval $nsamples]
        wonks-check statedata prog $prog
    } else {
        after idle {wonks-check goto idle}
    }
}

# go here when perftest has finished
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
wonks-check state update

# when moving to the update state, store the data from perftest
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
wonks-check transition checking->update -onchange {
    set data [wonks-check statedata perftable]
    set prog [wonks-check statedata prog]

    if {[catch {read $prog} msg] == 0} {
        if {[regexp {^([^ ]+) +([0-9]+)} $msg match vstr wval]} {
            set p2p::wonks::version $vstr
            foreach {load mem oldwval nsamples} [wonks-check statedata sysinfo] break
            if {$nsamples >= 1} {
                # average new value with old values
                set wval [expr {$wval/double($nsamples+1)
                                 + $nsamples/double($nsamples+1)*$oldwval}]
            }
            $data delete [list $load $mem -- --]
            $data add [list $load $mem $wval [incr nsamples]]
        } else {
            log error "ERROR: performance test failed: $msg"
        }
    }
    catch {close $prog}
    wonks-check statedata prog ""
    wonks-check statedata sysinfo ""

    after idle {wonks-check goto idle}
}

# ----------------------------------------------------------------------
#  COMMAND:  p2p::wonks::init
#
#  Invoked when a worker node starts up to initialize this part of
#  the package.  This triggers autoloading of the file, and also
#  initializes variables used by the functions in this file.
# ----------------------------------------------------------------------
proc p2p::wonks::init {} {
    variable version
    set version "?"

    set data [Rappture::tuples ::#auto]
    $data dimension add load1
    $data dimension add freeswap
    $data dimension add wonks
    $data dimension add nsamples
    wonks-check statedata perftable $data

    wonks-check goto idle
}

# ----------------------------------------------------------------------
#  COMMAND:  p2p::wonks::current
#
#  Used to estimate the current performance of this node as a number
#  of wonks.  Takes
# ----------------------------------------------------------------------
proc p2p::wonks::current {} {
    foreach {load mem} [Rappture::sysinfo load1 freeswap] break
    set load [format "%.1f" $load]
    set mem [expr {$mem/1048576}]  ;# in megabytes

    set data [wonks-check statedata perftable]
    if {[$data inrange [list $load $mem -- --]]} {
        set wval [$data interpolate wonks [list $load $mem -- --]]
        return [list $p2p::wonks::version [expr {round($wval)}]]
    }

    # don't have any data -- we're forced to exec perftest and block
    if {[catch {exec [file join $p2p::wonks::dir perftest]} msg] == 0
          && [regexp {^([^ ]+) +([0-9]+)} $msg match vstr wval]} {
        $data add [list $load $mem $wval 1]
        set p2p::wonks::version $vstr
        return [list $vstr $wval]
    }

    # can't even exec perftest -- return some large bogus value
    return [list bogus 1000000]
}
