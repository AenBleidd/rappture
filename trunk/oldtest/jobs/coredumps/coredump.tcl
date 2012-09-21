# ----------------------------------------------------------------------
#  EXAMPLE: Rappture core dumps
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Rappture

# open the XML file containing the run parameters
set driver [Rappture::library [lindex $argv 0]]

set type [$driver get input.(jobtype).current]
set sig [$driver get input.(signal).current]

proc killjob {signal} {
    set jobs [exec ps uxaw | grep -v grep | grep =[pid]]
    exec kill -s $signal [lindex $jobs 1]
}

if {$type == "fail"} {
    after [expr {int(rand()*5000)}] "killjob $sig"
}

if {[catch {Rappture::exec tclsh genoutput.tcl =[pid]} result]} {
    puts $result
    exit 1
}

$driver put output.log "Job succeeded:\n$result"

Rappture::result $driver
exit 0
