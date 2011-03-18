# ----------------------------------------------------------------------
#  mkobjects.tcl
#
#  This utility is similar to mkindex.tcl, but it searches for files
#  associated with Rappture object classes and adds them to the
#  existing tclIndex file, which should have been built by mkindex.tcl
#    USAGE:  tclsh mkobjects.tcl --srcdir <dir> <type> <type> ...
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2011  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
set outfile "tclIndex"
set srcdir  "."
set args {}
for {set i 0} {$i < [llength $argv]} {incr i} {
    set arg [lindex $argv $i]
    if {$arg eq "--srcdir"} {
	incr i
	set srcdir [lindex $argv $i]
	continue
    }
    lappend args $arg
}

if {![file exists $outfile]} {
    puts stderr "can't find tclIndex file to update"
    exit 1
}

set fid [open $outfile a]
puts $fid "\n# object viewers"
foreach type $args {
    foreach dir {input output} {
        set fname "$type$dir.tcl"
        set class "::Rappture::objects::[string totitle $type][string totitle $dir]"
        if {[file exists [file join $srcdir objects $type $fname]]} {
            puts $fid "set auto_index($class) \[list source \[file join \$dir objects $type $fname\]\]"
        }
    }
}
close $fid

exit 0
