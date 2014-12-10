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

# tasks in execute mode run quietly and don't try to save results
$TaskObj configure -jobstats "" -resultdir ""

# Transfer input values from driver to TaskObj, and then run.
# ----------------------------------------------------------------------

# copy inputs from the test into the run file
$TaskObj reset
foreach path [Rappture::entities -as path $driverobj input] {
    if {[$driverobj element -as type $path.current] ne ""} {
        lappend args $path [$driverobj get $path.current]
    }
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
$runxml put output.time [clock format [clock seconds]]

$runxml put tool.version.rappture.version $::Rappture::version
$runxml put tool.version.rappture.revision $::Rappture::build

if {[info exists tcl_platform(user)]} {
    $runxml put output.user $::tcl_platform(user)
}

puts [$runxml xml]
exit $status
