#!/bin/sh
# -*- mode: Tcl -*-
# ----------------------------------------------------------------------
#  RAPPTURE LAUNCHER
#
#  This script is invoked by the "rappture" command.  It parses
#  various input options and selects the proper main program to
#  run depending on the function (build tool, run tool, tester, etc.).
#
#  RUN AS FOLLOWS:
#    rappture ?-run? ?-tool <toolfile>?
#    rappture -builder ?-tool <toolfile>?
#    rappture -tester ?-tool <toolfile>? ?-testdir <directory>?
#
#  The default option is "-run", which brings up the GUI used to
#  run the tool.  If the <toolfile> is not specified, it defaults
#  to "tool.xml" in the current working directory.
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
set mainscript ""
set alist ""
set toolxml ""

# scan through the arguments and look for the function
while {[llength $argv] > 0} {
    set opt [lindex $argv 0]
    set argv [lrange $argv 1 end]

    if {[string index $opt 0] == "-"} {
        switch -- $opt {
            -run {
                package require RapptureGUI
                set guidir $RapptureGUI::library
                set mainscript [file join $guidir scripts main.tcl]
                set reqpkgs Tk
            }
            -builder {
                package require RapptureBuilder
                set blddir $RapptureBuilder::library
                set mainscript [file join $blddir scripts main.tcl]
                set reqpkgs Tk
            }
            -tester {
                package require RapptureTester
                set testdir $RapptureTester::library
                set mainscript [file join $testdir scripts main.tcl]
                set reqpkgs Tk
            }
            -tool {
                set toolxml [lindex $argv 0]
                set argv [lrange $argv 1 end]
                if {![file exists $toolxml]} {
                    puts stderr "file not found: $toolxml"
                    exit 1
                }
                lappend alist -tool $toolxml
            }
            -tool - -testdir - -nosim {
                lappend alist $opt [lindex $argv 0]
                set argv [lrange $argv 1 end]
            }
            -auto {
                # for the tester in automatic mode -- don't load Tk
                package require RapptureTester
                set testdir $RapptureTester::library
                set mainscript [file join $testdir scripts auto.tcl]
                set reqpkgs ""
            }
            -load {
                lappend alist $opt
                while { [llength $argv] > 0 } {
                    set val [lindex $argv 0]
                    if { [string index $val 0] == "-" } {
                        break
                    }
                    lappend alist $val
                    set argv [lrange $argv 1 end]
                }
            }
            default {
                puts stderr "usage:"
                puts stderr "  rappture ?-run? ?-tool toolFile? ?-nosim 0/1? ?-load file file ...?"
                puts stderr "  rappture -builder ?-tool toolFile?"
                puts stderr "  rappture -tester ?-auto? ?-tool toolFile? ?-testdir directory?"
                exit 1
            }
        }
    }
}

# If no arguments, assume that it's the -run option
if {$mainscript eq ""} {
    package require RapptureGUI
    set guidir $RapptureGUI::library
    set mainscript [file join $guidir scripts main.tcl]
    set reqpkgs Tk
}

# Invoke the main program with the args

# Note: We're sourcing the driver file "main.tcl" rather than exec-ing
#       wish because we want to see stderr and stdout messages when they
#	are written, rather than when the program completes.  It also 
#	eliminates one process waiting for the other to complete. If 
#	"exec" is needed, then the following could be replaced with 
#	blt::bgexec.  It doesn't try to redirect stderr into a file.
set argv $alist
foreach name $reqpkgs {
    package require $name
}
source  $mainscript
