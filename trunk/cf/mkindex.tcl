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

proc auto_mkindex { srcdir outfile patterns } {
    global errorCode errorInfo

    if {[interp issafe]} {
        error "can't generate index within safe interpreter"
    }

    set fid [open $outfile w]

    set oldDir [pwd]
    cd $srcdir
    set srcdir [pwd]

    append index "# Tcl autoload index file, version 2.0\n"
    append index "# This file is generated by the \"auto_mkindex\" command\n"
    append index "# and sourced to set up indexing information for one or\n"
    append index "# more commands.  Typically each line is a command that\n"
    append index "# sets an element in the auto_index array, where the\n"
    append index "# element name is the name of a command and the value is\n"
    append index "# a script that loads the command.\n\n"
    if {$patterns == ""} {
        set patterns *.tcl
    }
    auto_mkindex_parser::init
    foreach file [eval glob $patterns] {
        if {[catch {auto_mkindex_parser::mkindex $file} msg] == 0} {
            append index $msg
        } else {
            set code $errorCode
            set info $errorInfo
            cd $oldDir
            error $msg $info $code
        }
    }
    auto_mkindex_parser::cleanup
    puts -nonewline $fid $index
    close $fid
    cd $oldDir
}

set outfile "tclIndex"
set srcdir  "."
set args {}
for {set i 0} { $i < [llength $argv] } { incr i } {
    set arg [lindex $argv $i]
    if { $arg == "--outfile" } {
	incr i
	set outfile [lindex $argv $i]
	continue
    } elseif { $arg == "--srcdir" } {
	incr i
	set srcdir [lindex $argv $i]
	continue
    }
    lappend args $arg
}

auto_mkindex $srcdir $outfile $args
if { ![file exists $outfile] } {
    exit 1
} 
exit 0