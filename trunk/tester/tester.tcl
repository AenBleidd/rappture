#! /bin/sh
# ----------------------------------------------------------------------
# RAPPTURE REGRESSION TESTER
#
# This program will read a set of test xml files typically located in
# a tool's "tests" subdirectory, and provide an interactive test suite.
# The test xml files should contain a complete set of inputs and outputs
# for one run of an application.  In each test xml, a label must be
# located at the path test.label.  Test labels may be organized
# hierarchically by using dots to separate components of the test label
# (example: roomtemp.1eV).  A description may optionally be located at
# the path test.description.  Input arguments are the path to the
# tool.xml of the version being tested, and the path the the directory
# containing a set of test xml files.  If the arguments are missing,
# the program will attempt to locate them automatically.
#
# USAGE: tester.tcl ?-tool tool.xml? ?-testdir tests?
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

package require Tk
package require Rappture
package require RapptureGUI

wm withdraw .

Rappture::getopts argv params {
    value -tool ""
    value -testdir ""
}

# If tool.xml and test directory locations are not given, try to find them.
if {$params(-tool) == ""} {
    if {[file exists tool.xml]} {
        set params(-tool) tool.xml
    } elseif {[file exists [file join rappture tool.xml]]} {
        set params(-tool) [file join rappture tool.xml]
    } else {
        error "Cannot find tool.xml"
    }
}

if {$params(-testdir) == ""} {
    set tooldir [file dirname $params(-tool)]
    if {[file isdirectory [file join $tooldir tests]]} {
        set params(-testdir) [file join $tooldir tests]
    } elseif {[file isdirectory [file join [file dirname $tooldir] tests]]} {
        set params(-testdir) [file join [file dirname $tooldir] tests]
    } else {
        error "Cannot find test directory."
    }
}

Rappture::Tester::MainWin .main $params(-tool) $params(-testdir)
wm protocol .main WM_DELETE_WINDOW exit
wm title .main "Rappture Regression Tester"

