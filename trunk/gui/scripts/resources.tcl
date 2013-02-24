# -*- mode: tcl; indent-tabs-mode: nil -*- 
# ----------------------------------------------------------------------
#  COMPONENT: resources
#
#  This file contains routines to parse settings from the "resources"
#  file specified by the middleware.  This file is located via an
#  environment variable as $SESSIONDIR/resources.  If this file
#  exists, it is used to load settings that affect the rest of the
#  application.  It looks something like this:
#
#    application_name "My Tool"
#    results_directory /tmp
#    filexfer_port 6593
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itcl

namespace eval Rappture { # forward declaration }
namespace eval Rappture::resources {
    #
    # Set up a safe interpreter for loading filexfer options...
    #
    variable optionParser [interp create -safe]
    foreach cmd [$optionParser eval {info commands}] {
        $optionParser hide $cmd
    }
    # this lets us ignore unrecognized commands in the file:
    $optionParser invokehidden proc unknown {args} {}
}

# ----------------------------------------------------------------------
# USAGE: Rappture::resources::register ?<name> <proc> ...?
#
# Used by other files throughout the Rappture GUI to register their
# own commands for the resources parser.  Creates a command <name>
# in the resources interpreter, and registers the <proc> to handle
# that command.
# ----------------------------------------------------------------------
proc Rappture::resources::register {args} {
    variable optionParser
    foreach {name proc} $args {
        $optionParser alias $name $proc
    }
}

# ----------------------------------------------------------------------
# USAGE: Rappture::resources::load ?<errorCallback>?
#
# Called in the main program to load resources from the file
# $SESSIONDIR/resources.  As a side effect of loading resources,
# the data is stored in various variables throughout the package.
#
# Returns 1 if successful, and 0 otherwise.  Any errors are passed
# along to the <errorCallback>.  If not specified, then the default
# callback pops up a message box.
# ----------------------------------------------------------------------
proc Rappture::resources::load {{callback tk_messageBox}} {
    global env
    variable optionParser

    #
    # First, make sure that we've loaded all bits of code that
    # might register commands with the resources interp.  These
    # are all in procs named xxx_init_resources.
    #
    global auto_index
    foreach name [array names auto_index *_init_resources] {
        eval $name
    }

    #
    # Look for a $SESSIONDIR variable and a file called
    # $SESSIONDIR/resources.  If found, then load the settings
    # from that file.
    #
    if {[info exists env(SESSIONDIR)]} {
        set file $env(SESSIONDIR)/resources
        if {![file exists $file]} {
            return 0
        }

        if {[catch {
            set fid [open $file r]
            set info [read $fid]
            close $fid
            $optionParser eval $info
        } result]} {
            if {"" != $callback} {
                after 1 [list $callback -title Error -icon error -message "Error in resources file:\n$result"]
            }
            return 0
        }
    }
    return 1
}
