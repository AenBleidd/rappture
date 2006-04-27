# ----------------------------------------------------------------------
#  COMPONENT: icons - utility for loading icons from a library
#
#  This utility makes it easy to load GIF and XBM files installed
#  in a library in the final installation.  It is used throughout
#  the Rappture GUI, whenever an icon is needed.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================

namespace eval RapptureGUI {
    variable iconpath [file join $library scripts images]
}

# ----------------------------------------------------------------------
# USAGE: Rappture::icon <name>
#
# Searches for an icon called <name> on all of the directories in
# the search path set up by RapptureGUI::iconpath.
# ----------------------------------------------------------------------
proc Rappture::icon {name} {
    variable ::RapptureGUI::iconpath
    variable ::RapptureGUI::icons

    #
    # Already loaded? then return it directly
    #
    if {[info exists icons($name)]} {
        return $icons($name)
    }

    #
    # Search for the icon along the iconpath search path
    #
    set file ""
    foreach dir $iconpath {
        set path [file join $dir $name.*]
        set file [lindex [glob -nocomplain $path] 0]
        if {"" != $file} {
            break
        }
    }

    set imh ""
    if {"" != $file} {
        switch -- [file extension $file] {
            .gif - .jpg {
                set imh [image create photo -file $file]
            }
            .xbm {
                set fid [open $file r]
                set data [read $fid]
                close $fid
                set imh bitmap-$name
                blt::bitmap define $imh $data
            }
        }
    }
    if {"" != $imh} {
        set icons($name) $imh
    }
    return $imh
}
