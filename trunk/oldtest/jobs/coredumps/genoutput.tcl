# ----------------------------------------------------------------------
#  EXAMPLE: Rappture core dumps
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================

# spit out a bunch of junk until we get killed
set n 10
while {[incr n -1] > 0} {
    puts [exec fortune]
    after [expr {int(rand()*rand()*3000)}]
}
