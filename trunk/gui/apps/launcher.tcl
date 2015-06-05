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
#    rappture -execute driver.xml ?-tool <toolfile>?
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
set loadlist ""
set toolxml ""

# ----------------------------------------------------------------------
#  Look for parameters passed into the tool session.  If there are
#  any "file" parameters, they indicate files that should be loaded
#  for browsing or executed to get results:
#
#    file(load):/path/to/run.xml
#    file(execute):/path/to/driver.xml
# ----------------------------------------------------------------------
set params(opt) ""
set params(load) ""
set params(execute) ""
set params(input) ""

if {[info exists env(TOOL_PARAMETERS)]} {
    # if we can't find the file, wait a little
    set ntries 25
    while {$ntries > 0 && ![file exists $env(TOOL_PARAMETERS)]} {
        after 200
        incr ntries -1
    }

    if {![file exists $env(TOOL_PARAMETERS)]} {
        # still no file after all that? then skip parameters
        puts stderr "WARNING: can't read tool parameters in file \"$env(TOOL_PARAMETERS)\"\nFile not found."

    } elseif {[catch {
        # read the file and parse the contents
        set fid [open $env(TOOL_PARAMETERS) r]
        set info [read $fid]
        close $fid
    } result] != 0} {
        puts stderr "WARNING: can't read tool parameters in file \"$env(TOOL_PARAMETERS)\"\n$result"

    } else {
        # parse the contents of the tool parameter file
        foreach line [split $info \n] {
            set line [string trim $line]
            if {$line eq "" || [regexp {^#} $line]} {
                continue
            }

            if {[regexp {^([a-zA-Z]+)(\(into\:)(.+)\)\:(.+)$} $line match type name path value]
                || [regexp {^([a-zA-Z]+)(\([^)]+\))?\:(.+)} $line match type name value]} {
                if {$type eq "file"} {
                    switch -exact -- $name {
                        "(load)" - "" {
                            lappend params(load) $value
                            set params(opt) "-load"
                        }
                        "(execute)" {
                            set params(execute) $value
                            set params(opt) "-execute"
                        }
                        "(input)" {
                            set params(input) $value
                            set params(opt) "-input"
                        }
                        "(into:" {
                            namespace eval ::Rappture { # forward decl }
                            set ::Rappture::parameters($path) $value
                        }
                        default {
                            puts stderr "WARNING: directive $name not recognized for file parameter \"$value\""
                        }
                    }
                }
            }
        }
    }
}

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
            -execute {
                # for web services and simulation cache -- don't load Tk
                set reqpkgs ""
                if {[llength $argv] < 1} {
                    puts stderr "error: missing driver.xml file for -execute option"
                    exit 1
                }
                set driverxml [lindex $argv 0]
                set argv [lrange $argv 1 end]

                if {![file readable $driverxml]} {
                    puts stderr "error: driver file \"$driverxml\" not found"
                    exit 1
                }

                set dir [file dirname [info script]]
                set mainscript [file join $dir execute.tcl]
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
            -testdir - -nosim {
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
                while { [llength $argv] > 0 } {
                    set val [lindex $argv 0]
                    if { [string index $val 0] == "-" } {
                        break
                    }
                    lappend loadlist $val
                    set argv [lrange $argv 1 end]
                }
            }
            default {
                puts stderr "usage:"
                puts stderr "  rappture ?-run? ?-tool toolFile? ?-nosim 0/1? ?-load file file ...?"
                puts stderr "  rappture -builder ?-tool toolFile?"
                puts stderr "  rappture -tester ?-auto? ?-tool toolFile? ?-testdir directory?"
                puts stderr "  rappture -execute driver.xml ?-tool toolFile?"
                exit 1
            }
        }
    }
}

# If no arguments, check to see if there are any tool parameters.
# If not, then assume that it's the -run option.
if {$mainscript eq ""} {
    switch -- $params(opt) {
        -load {
            # add tool parameters to the end of any files given on cmd line
            set loadlist [concat $loadlist $params(load)]
            set alist [concat $alist -load $loadlist]

            package require RapptureGUI
            set guidir $RapptureGUI::library
            set mainscript [file join $guidir scripts main.tcl]
            set reqpkgs Tk
        }
        -execute {
            if {[llength $params(execute)] != 1} {
                puts stderr "ERROR: wrong number of (execute) files in TOOL_PARAMETERS (should be only 1)"
                exit 1
            }
            set driverxml [lindex $params(execute) 0]
            if {![file readable $driverxml]} {
                puts stderr "error: driver file \"$driverxml\" not found"
                exit 1
            }
            set dir [file dirname [info script]]
            set mainscript [file join $dir execute.tcl]
            set reqpkgs ""

            # When executing from TOOL_PARAMETERS file directives,
            # report status, clean up, and save output to data/results.
            # This helps the web services interface do its thing.
            set alist [list \
                -output @default \
                -status rappture.status \
                -cleanup yes]
        }
        "" - "-input" {
            package require RapptureGUI
            set guidir $RapptureGUI::library
            set mainscript [file join $guidir scripts main.tcl]
            set reqpkgs Tk

            # finalize the -input argument for "rappture -run"
            if {$params(input) ne ""} {
                if {![file readable $params(input)]} {
                    puts stderr "error: driver file \"$params(input)\" not found"
                    exit 1
                }
                set alist [concat $alist -input $params(input)]
            }

            # finalize any pending -load arguments for "rappture -run"
            if {[llength $loadlist] > 0} {
                set alist [concat $alist -load $loadlist]
            }
        }
        default {
            puts stderr "internal error: funny action \"$params(opt)\" inferred from TOOL_PARAMETERS"
            exit 1
        }
    }
} else {
    # finalize any pending -load arguments for "rappture -run"
    if {[llength $loadlist] > 0} {
        set alist [concat $alist -load $loadlist]
    }
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
