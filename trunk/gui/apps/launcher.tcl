#!/bin/sh
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
#  Copyright (c) 2004-2011  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require RapptureGUI
set guidir $RapptureGUI::library

package require RapptureBuilder
set blddir $RapptureBuilder::library

package require RapptureTester
set testdir $RapptureTester::library

set mainscript [file join $guidir scripts main.tcl]
set alist ""
set toolxml ""

# scan through the arguments and look for the function
while {[llength $argv] > 0} {
    set opt [lindex $argv 0]
    set argv [lrange $argv 1 end]

    if {[string index $opt 0] == "-"} {
        switch -- $opt {
            -run {
                set mainscript [file join $guidir scripts main.tcl]
            }
            -builder {
                set mainscript [file join $blddir scripts main.tcl]
            }
            -tester {
                set mainscript [file join $testdir scripts main.tcl]
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
                puts stderr "  rappture -tester ?-tool toolFile? ?-testdir directory?"
                exit 1
            }
        }
    }
}

# invoke the main program with the args
puts stderr alist=$alist
eval exec wish [list $mainscript] $alist
