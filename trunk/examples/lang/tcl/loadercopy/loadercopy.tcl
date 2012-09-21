# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <loader> elements
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Rappture

# open the XML file containing the run parameters
set driver [Rappture::library [lindex $argv 0]]

set file1 [$driver get input.group(tabs).group(layer1).loader.file]
set file2 [$driver get input.group(tabs).group(layer2).loader.file]

$driver put output.log "file1 = $file1\nfile2 = $file2"

# save the updated XML describing the run...
Rappture::result $driver
exit 0
