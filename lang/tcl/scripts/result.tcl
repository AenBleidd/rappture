# ----------------------------------------------------------------------
#  COMPONENT: result - use this to report results
#
#  This utility makes it easy to report results at the end of a
#  run within a simulator.  It takes the XML object containing the
#  inputs and outputs and writes it out to a "run" file with an
#  automatically generated name.  Then, it writes out the name
#  of the run file as "=RAPPTURE-RUN=>name" so the Rappture GUI
#  knows the name of the result file.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================

namespace eval Rappture { # forward declaration }

# ----------------------------------------------------------------------
# USAGE: result <libraryObj> ?<status>?
#
# This utility takes the <libraryObj> representing a run (driver file
# plus outputs) and writes it out to the run file.  It should be called
# within a simulator at the end of simulation, to communicate back
# the results.
#
# If the optional <status> is specified, then it represents the exit
# status code for the tool.  "0" means "ok" and anything non-zero means
# "failed".
# ----------------------------------------------------------------------
proc Rappture::result {libobj {status 0}} {
    global tcl_platform

    $libobj put output.time [clock format [clock seconds]]
    if {$status != 0} {
        $libobj put output.status "failed"
    } else {
        $libobj put output.status "ok"
    }

    # I can't stand that there are two versions of this.  I spend more time
    # tracking down problems that exist in the one version but not the other.
    # For example, it matters if the tool is running TCL or some other
    # language.

    $libobj put tool.version.rappture.version $::Rappture::version
    $libobj put tool.version.rappture.revision $::Rappture::build
    $libobj put tool.version.rappture.modified "\$LastChangedDate$"
    $libobj put tool.version.rappture.language "tcl"

    if {[info exists tcl_platform(user)]} {
        $libobj put output.user $tcl_platform(user)
    }

    # fake milliseconds by using clock clicks
    # fake microseconds by using 000
    # this will be fixed when tcl uses C bindings
    set timestamp [format %d%03d%03d [clock seconds] [expr [clock clicks -milliseconds]%1000] 0]
    set oname "run$timestamp.xml"
    set fid [open $oname w]
    puts $fid "<?xml version=\"1.0\"?>"
    puts $fid [$libobj xml]
    close $fid

    if {$status == 0} {
        puts "=RAPPTURE-RUN=>$oname"
    }
}
