# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <string> elements -- binary data
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

set data [$driver get input.(tarball).current]

set file "tar[pid].tgz"
set fid [open $file w]
fconfigure $fid -translation binary -encoding binary
puts -nonewline $fid $data
close $fid

set status [catch {exec tar tvzf $file} result]
file delete -force $file

if {$status != 0} {
    puts stderr "ERROR: $result"
    exit 1
}

$driver put output.string(dir).about.label "Contents"
$driver put output.string(dir).current $result

$driver put output.string(tarball).about.label "Original Tar File"
$driver put output.string(tarball).current $data
$driver put output.string(tarball).filetype ".tgz"

# save the updated XML describing the run...
Rappture::result $driver
exit 0
