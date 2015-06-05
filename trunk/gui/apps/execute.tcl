#! /bin/sh
# ----------------------------------------------------------------------
#  RAPPTURE PROGRAM EXECUTION
#
#  This script implements the -execute option for Rappture.  It
#  provides a way to run a specific Rappture simulation without
#  invoking the Rappture GUI.  Instead, it takes a driver file with
#  the required parameters, submits that for execution, and then
#  returns the run.xml file.  If the -tool file is specified, then
#  it double-checks the driver against the tool to make sure that
#  the requested tool and version are compatible.
#
#  This is normally invoked by launcher.tcl, so it expects $driverxml
#  and $toolxml variable to be already set.
#
#  USAGE: execute.tcl
# ======================================================================
#  AUTHORS:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
#\
exec tclsh "$0" $*
# ----------------------------------------------------------------------
# tclsh executes everything from here on...

# bring in the Rappture object system
package require Rappture
Rappture::objects::init
Rappture::resources::load

# load the XML info in the driver file
if {[catch {Rappture::library $driverxml} result]} {
    puts stderr "ERROR while loading driver file \"$driverxml\""
    puts stderr $result
    exit 1
}
set driverobj $result

# If tool.xml is not specified, try to find it the way Rappture would.
if {$toolxml eq ""} {
    if {[file isfile tool.xml]} {
        set toolxml [file normalize tool.xml]
    } elseif {[file isfile [file join rappture tool.xml]]} {
        set toolxml [file normalize [file join rappture tool.xml]]
    }
}

# If there's still no tool.xml, then see if we can find tooldir in driver
if {$toolxml eq ""} {
    set tooldir [$driverobj get tool.version.application.directory(tool)]
    if {$tooldir eq ""} {
        puts stderr "ERROR: missing -tool option, and driver file doesn't contain sufficient detail to locate the desired tool."
        exit 1
    }

    set toolxml [file join $tooldir tool.xml]
    if {![file exists $toolxml]} {
        puts stderr "ERROR: missing tool.xml file \"$toolxml\""
        exit 1
    }
}

set installdir [file dirname [file normalize $toolxml]]
set toolobj [Rappture::library $toolxml]
set TaskObj [Rappture::Task ::#auto $toolobj $installdir]
set LogFid ""

# Define some things that we need for logging status...
# ----------------------------------------------------------------------
proc log_output {message} {
    global LogFid

    if {$LogFid ne ""} {
        #
        # Scan through and pick out any =RAPPTURE-PROGRESS=> messages.
        #
        set percent ""
        while {[regexp -indices \
                {=RAPPTURE-PROGRESS=> *([-+]?[0-9]+) +([^\n]*)(\n|$)} $message \
                 match percent mesg]} {

            foreach {i0 i1} $percent break
            set percent [string range $message $i0 $i1]

            foreach {i0 i1} $mesg break
            set mesg [string range $message $i0 $i1]

            foreach {i0 i1} $match break
            set message [string replace $message $i0 $i1]
        }
        if {$percent ne ""} {
            # report the last percent progress found
            log_append progress "$percent% - $mesg"
        }
    }
}

# Actually write to the log file
proc log_append {level message} {
    global LogFid

    if {$LogFid ne ""} {
        set date [clock format [clock seconds] -format {%Y-%m-%dT%H:%M:%S%z}]
        set host [info hostname]
        puts $LogFid "$date $host rappture [pid] \[$level\] $message"
        flush $LogFid
    }
}

# Actually write to the log file
proc log_stats {args} {
    set line ""
    foreach {key val} $args {
        append line "$key=$val "
    }
    log_append usage $line
}

# Parse command line options to see
# ----------------------------------------------------------------------
Rappture::getopts argv params {
    value -status ""
    value -output ""
    value -cleanup no
}

if {$params(-status) ne ""} {
    set LogFid [open $params(-status) w]
    $TaskObj configure -logger {log_append status} -jobstats log_stats
}

if {$params(-output) eq ""} {
    # no output? then run quietly and don't try to save results
    $TaskObj configure -jobstats "" -resultdir ""
}

# Transfer input values from driver to TaskObj, and then run.
# ----------------------------------------------------------------------

# copy inputs from the test into the run file
$TaskObj reset
foreach path [Rappture::entities -as path $driverobj input] {
    if {[$driverobj element -as type $path.current] ne ""} {
        lappend args $path [$driverobj get $path.current]
    }
}

if {$params(-status) ne ""} {
    # recording status? then look through output for progress messages
    lappend args -output log_output
}

# run the desired case...
foreach {status result} [eval $TaskObj run $args] break

if {$status == 0 && [Rappture::library isvalid $result]} {
    set runxml $result
    $runxml put output.status ok
} else {
    # build a run file for the result output
    set info "<?xml version=\"1.0\"?>\n[$driverobj xml]"
    set runxml [Rappture::LibraryObj ::#auto $info]
    $runxml put output.log $result
    $runxml put output.status failed
}

# Handle output
# ----------------------------------------------------------------------
switch -- $params(-output) {
    "" {
        # no output file -- write to stdout
        puts "<?xml version=\"1.0\"?>\n[$runxml xml]"
    }
    "@default" {
        # do the usual Rappture thing -- move to results dir
        # but ignore any errors if it fails
        catch {$TaskObj save $runxml}
    }
    default {
        # save to the specified file
        $TaskObj save $runxml $params(-output)
    }
}

if {$params(-cleanup)} {
    file delete -force -- $driverxml
}

log_append status "exit $status"
exit $status
