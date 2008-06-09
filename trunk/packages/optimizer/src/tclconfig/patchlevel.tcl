# ----------------------------------------------------------------------
#  patchlevel.tcl
#
#  This utility determines the current patchlevel for this package
#  by querying the revision level from Subversion.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
if {[catch {exec svn info} result] == 0
      && [regexp {Revision: +([0-9]+)} $result match num]} {
    puts "r$num"
} else {
    puts "r?"
}
exit 0
