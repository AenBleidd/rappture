#! /bin/sh
# ----------------------------------------------------------------------
#  RAPPTURE REGRESSION TESTER
#
#  This program will read a set of test xml files typically located in
#  a tool's "tests" subdirectory, and provide an interactive test suite.
#  The test xml files should contain a complete set of inputs and 
#  outputs for one run of an application.  In each test xml, a label 
#  must be located at the path test.label.  Test labels may be organized
#  hierarchically by using dots to separate components of the test label
#  (example: roomtemp.1eV).  A description may optionally be located at
#  the path test.description.  Input arguments are the path to the
#  tool.xml of the version being tested, and the path the the directory
#  containing a set of test xml files.  If the arguments are missing,
#  the program will attempt to locate them automatically.
#
#  USAGE: tester.tcl ?-tool tool.xml? ?-testdir tests?
# ======================================================================
#  AUTHOR:  Ben Rafferty, Purdue University
#  Copyright (c) 2010  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
#\
exec tclsh "$0" $*
# ----------------------------------------------------------------------
# wish executes everything from here on...

# TODO: Won't need this once tied in with the rest of the package 
lappend auto_path [file dirname $argv0]

package require Itk
package require Rappture
package require RapptureGUI

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
# If given test directory does not exist, create it.
} elseif {![file exists $params(-testdir)]} {
    file mkdir $params(-testdir)
} else {
    puts "Non-directory file exists at test location \"$params(-testdir)\""
    exit 1
}

# ----------------------------------------------------------------------
# INITIALIZE WINDOW
# ----------------------------------------------------------------------
wm title . "Rappture Regression Tester"

menu .mb
menu .mb.file -tearoff 0
.mb add cascade -label "File" -underline 0 -menu .mb.file
.mb.file add command -label "Select tool" -underline 7 \
    -command {set params(-tool) [tk_getOpenFile]
              .tree configure -toolxml $params(-tool)}
.mb.file add command -label "Select test directory" -underline 12 \
    -command {set params(-testdir) [tk_chooseDirectory]
              .tree configure -testdir $params(-testdir)}
.mb.file add command -label "Exit" -underline 1 -command {destroy .}
. configure -menu .mb

panedwindow .pw

.pw add [Rappture::Tester::TestTree .tree \
    -testdir $params(-testdir) \
    -toolxml $params(-tool) \
    -selectcommand Rappture::Tester::selectionHandler]

.pw add [frame .right]
Rappture::Tester::TestView .right.view
button .right.regoldenize -text "Regoldenize" -state disabled \
    -command Rappture::Tester::regoldenize
pack .right.regoldenize -side bottom -anchor e
pack .right.view -side bottom -expand yes -fill both

pack .pw -expand yes -fill both

set lastsel ""

# TODO: Handle resizing better

# ----------------------------------------------------------------------
# USAGE: selectionHandler ?-refresh?
#
# Used internally to communicate between the test tree and the right
# hand side viewer.  Upon selecting a new tree node, pass the focused
# node's data to the right hand side.  Use the -refresh option to force
# the selected test to be re-displayed on the right side.
# ----------------------------------------------------------------------
proc Rappture::Tester::selectionHandler {args} {
    global lastsel
    set test [.tree getTest]
    if {$test != $lastsel || [lsearch $args "-refresh"] != -1} {
        .right.view configure -test $test
        if {$test != "" && [$test hasRan] && [$test getResult] != "Error"} {
            .right.regoldenize configure -state normal
        } else {
            .right.regoldenize configure -state disabled
        }
        set lastsel $test
    }
}

# ----------------------------------------------------------------------
# USAGE: regoldenize
#
# Regoldenizes the currently focused test case.  Displays a warning
# message.  If confirmed, copy the test information from the existing
# test xml into the new result, and write the new data into the test
# xml.
# ----------------------------------------------------------------------
proc Rappture::Tester::regoldenize {} {
    set test [.tree getTest]
    set testxml [$test getTestxml]
    if {[tk_messageBox -type yesno -icon warning -message "Are you sure you want to regoldenize?\n$testxml will be overwritten."]} {
        $test regoldenize
        .tree refresh 
        selectionHandler -refresh
    }
}

