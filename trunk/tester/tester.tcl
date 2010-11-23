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
# the path test.description.
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

# TODO: Use tclIndex to manage classes correctly
source mainwin.tcl
source testtree.tcl
source testview.tcl
source compare.tcl

wm withdraw .

set testdir "example/tests"
set tooldir "example"

::Rappture::Regression::MainWin .main $tooldir $testdir
bind .main <Destroy> {exit}

