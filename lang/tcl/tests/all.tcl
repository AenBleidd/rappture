# all.tcl --
#
# This file contains a top-level script to run all of the Tcl
# tests.  Execute it by invoking "source all.test" when running tcltest
# in this directory.
#
# Copyright (c) 1998-1999 by Scriptics Corporation.
# Copyright (c) 2000 by Ajuba Solutions
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# 
# RCS: @(#) $Id: all.tcl,v 1.16 2002/04/10 19:57:15 hobbs Exp $

set tcltestVersion [package require tcltest]
namespace import -force tcltest::*

if {$tcl_platform(platform) == "macintosh"} {
	tcltest::singleProcess 1
    set env(LD_LIBRARY_PATH) /usr/local/rappture/lib
    load [pwd]/../src/Rappture1.1.so
} else {
    set env(LD_LIBRARY_PATH) /usr/local/rappture/lib
    load [pwd]/../src/Rappture1.1.so
}
lappend auto_path [pwd]/../../../tcl/scripts/tmp.build \
	[pwd]/../../../gui/scripts/tmp.build

tcltest::testsDirectory [file dir [info script]]
tcltest::runAllTests

return
