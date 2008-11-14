# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <load><run> output elements
# ======================================================================
#  AUTHOR: Derrick S. Kearney, Purdue University
#  Copyright (c) 2005-2008  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Rappture

# open the XML file containing the run parameters
set driver [Rappture::library [lindex $argv 0]]

set type [$driver get input.choice(importtype).current]
set run1 [$driver get input.choice(first).current]
set run2 [$driver get input.choice(second).current]

$driver put output.log "hi mom"

if { ("load" == $type) || ("both" == $type) } {
    # test load.run
    $driver put output.load.run(run1) $run1
    $driver put output.load.run(run2) $run2
}

if { ("include" == $type) || ("both" == $type) } {
    # test include.run
    $driver put output.include.run(run1) $run1
    $driver put output.include.run(run2) $run2
}

# save the updated XML describing the run...
Rappture::result $driver
exit 0
