# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <string> elements -- binary data
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Rappture

# open the XML file containing the run parameters
set driver [Rappture::library [lindex $argv 0]]

set data [$driver get input.(tarball).current]

set file "tar[pid].tgz"
set fid [open $file w]
fconfigure $fid -translation binary -encoding binary
puts -nonewline $fid $data
close $fid

catch {exec tar tvzf $file} dir
file delete -force $file

$driver put output.string(dir).about.label "Contents"
$driver put output.string(dir).current $dir

# save the updated XML describing the run...
Rappture::result $driver
exit 0
