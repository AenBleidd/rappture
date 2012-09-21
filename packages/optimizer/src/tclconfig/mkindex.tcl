# ----------------------------------------------------------------------
#  mkindex.tcl
#
#  This utility freshens up the tclIndex file in a scripts directory.
#    USAGE:  tclsh mkindex.tcl ?<directory> <directory> ...?
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itcl  ;# include itcl constructs in the index

foreach dir $argv {
    auto_mkindex $dir *.tcl *.itcl
}
