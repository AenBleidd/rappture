# ----------------------------------------------------------------------
#  LIBRARY: randomization routines
# ----------------------------------------------------------------------
#  Michael McLennan (mmclennan@purdue.edu)
# ======================================================================
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================

expr srand([clock seconds])

# ----------------------------------------------------------------------
#  USAGE: randomize <list> ?<pickThisMany>?
#
#  Scrambles the elements in a list and returns them in random order.
#  Given a list of peer connections, for example, this function
#  scrambles the order so that a "foreach" command can traverse them
#  in a random order.  If <pickThisMany> is specified, then the list
#  is truncated after that many random entries.  Instead of randomizing
#  all 10,000 entries, for example, you can pick the first random 10.
# ----------------------------------------------------------------------
proc randomize {entries {pickThisMany -1}} {
    set rlist ""
    set rlen [llength $entries]
    for {set i 0} {$i < $rlen} {incr i} {
        # if we have the desired number of elements, then quit
        if {$pickThisMany >= 0 && $i >= $pickThisMany} {
            break
        }

        # pick a random element and add it to the return list
        set nrand [expr {int(rand()*[llength $entries])}]
        lappend rlist [lindex $entries $nrand]
        set entries [lreplace $entries $nrand $nrand]
    }
    return $rlist
}

# ----------------------------------------------------------------------
#  USAGE: random <list>
#
#  Picks one element at random from the given <list>.
# ----------------------------------------------------------------------
proc random {entries} {
    set nrand [expr {int(rand()*[llength $entries])}]
    return [lindex $entries $nrand]
}
