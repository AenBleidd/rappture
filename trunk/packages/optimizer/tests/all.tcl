# ----------------------------------------------------------------------
#  ALL TESTS
#
#  This file drives the entire test suite for the RapptureOptimizer
#  package.
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
#
set tcltestVersion [package require tcltest]
namespace import -force tcltest::*

if {$tcl_platform(platform) == "macintosh"} {
    tcltest::singleProcess 1
}

tcltest::testsDirectory [file dir [info script]]
tcltest::runAllTests

return
