#! /bin/sh
# ----------------------------------------------------------------------
#  RAPPTURE REGRESSION TESTER
#
#  This program will read a set of test xml files typically located in
#  a tool's "tests" subdirectory, and provide an automatic test suite.
#  Similar to main.tcl, but runs all tests automatically and exits with
#  status code zero if all tests pass.
#
#  USAGE: auto.tcl ?-tool tool.xml? ?-testdir tests?
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

set testerdir [file dirname [file normalize [info script]]]
lappend auto_path $testerdir

# bring in the Rappture object system
package require Rappture
Rappture::objects::init
Rappture::resources::load

Rappture::getopts argv params {
    value -tool ""
    value -testdir ""
}

# If tool.xml and test directory locations are not given, try to find them.
if {$params(-tool) == ""} {
    if {[file isfile tool.xml]} {
        set params(-tool) tool.xml
    } elseif {[file isfile [file join rappture tool.xml]]} {
        set params(-tool) [file join rappture tool.xml]
    } else {
        puts "Cannot find tool.xml"
        exit 1
    }
} elseif {![file isfile $params(-tool)]} {
    puts "Tool \"$params(-tool)\" does not exist"
    exit 1
}

if {$params(-testdir) == ""} {
    set tooldir [file dirname $params(-tool)]
    if {[file isdirectory [file join $tooldir tests]]} {
        set params(-testdir) [file join $tooldir tests]
    } elseif {[file isdirectory [file join [file dirname $tooldir] tests]]} {
        set params(-testdir) [file join [file dirname $tooldir] tests]
    } else {
        puts "Cannot find test directory"
        exit 1
    }
} elseif {![file isdirectory $params(-testdir)]} {
    puts "Test directory \"$params(-testdir)\" does not exist"
    exit 1
}

set installdir [file dirname [file normalize $params(-tool)]]
set xmlobj [Rappture::library $params(-tool)]
set TaskObj [Rappture::Task ::#auto $xmlobj $installdir]

# tasks in the tester run quietly and discard results
$TaskObj configure -jobstats "" -resultdir ""

# Load all tests in the test directory
# ----------------------------------------------------------------------
set testlist [glob -nocomplain -directory $params(-testdir) *.xml]
if {[llength $testlist] == 0} {
    puts stderr "ERROR:  no tests found in $params(-testdir)"
    exit 1
}

set nerrors 0
foreach file [glob -nocomplain -directory $params(-testdir) *.xml] {
    set testobj [Rappture::Tester::Test ::#auto $TaskObj $file]
    if {[$testobj getTestInfo test.label] eq ""} {
        puts stderr "ERROR:  Missing test label in $file"
        incr nerrors
        continue
    }

    set status [$testobj run]

    if {$status ne "finished"} {
        puts stderr "ERROR:  test $file didn't finish -- $status"
        incr nerrors
        continue
    }
    if {[$testobj getResult] ne "Pass"} {
        set diffs [$testobj getDiffs]
        puts stderr "FAILED:  $file ([llength $diffs] differences)"
        incr nerrors
        continue
    }
}

if {$nerrors > 0} {
    set errs [expr {($nerrors == 1) ? "ERROR" : "ERRORS"}]
    puts stderr "\n$nerrors $errs TOTAL"
}
