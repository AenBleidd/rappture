# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: logger - log user activities within the Rappture program
#
#  This library is used throughout a Rappture application to log
#  things that the user does.  This is useful for debugging, and also
#  for studying the effectiveness of simulation in education.  The
#  information is logged only if the RAPPTURE_LOG environment variable
#  is set.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
namespace eval Rappture::Logger {
    # by default, the logger is off
    variable enabled 0

    # log file name
    variable fileName ""

    # all activitiy is logged to this file channel
    variable fid ""
}

#
# HACK ALERT!  Many of the BLT graph-related widgets rely on BLT
#   bindings for zoom.  If we want to log these, we need to catch
#   them at the push/pop zoom level.  We do this once, here, by
#   wrapping the usual BLT procs in logging procs.
#
package require BLT
auto_load Blt_ZoomStack

rename ::blt::PushZoom ::blt::RealPushZoom
proc ::blt::PushZoom {graph} {
    # do the BLT call first
    ::blt::RealPushZoom $graph

    # now, log the results
    foreach {x0 x1} [$graph axis limits x] break
    foreach {y0 y1} [$graph axis limits y] break
    ::Rappture::Logger::log zoomrect -in $x0,$y0 $x1,$y1
}

rename ::blt::PopZoom ::blt::RealPopZoom
proc ::blt::PopZoom {graph} {
    # do the BLT call first
    ::blt::RealPopZoom $graph

    # now, log the results
    foreach {x0 x1} [$graph axis limits x] break
    foreach {y0 y1} [$graph axis limits y] break
    ::Rappture::Logger::log zoomrect -out $x0,$y0 $x1,$y1
}

# ----------------------------------------------------------------------
# USAGE: init ?on|off|auto?
#
# Called within the main program to initialize the logger package.
# Can be given the value "on" or "off" to turn logging on/off, or
# "auto" to rely on the RAPPTURE_LOG environment variable to control
# logging.  The default is "auto".
# ----------------------------------------------------------------------
proc Rappture::Logger::init {{state "auto"}} {
    global env tcl_platform
    variable enabled
    variable fileName
    variable fid

    if {$state eq "auto"} {
        set state "off"
        if {[info exists env(RAPPTURE_LOG)] && $env(RAPPTURE_LOG) ne ""} {
            if {[file isdirectory $env(RAPPTURE_LOG)]} {
                set state "on"
            } else {
                error "can't log: RAPPTURE_LOG directory does not exist"
            }
        }
    }
    if {![string is boolean -strict $state]} {
        error "bad value \"$state\": should be on, off, or auto"
    }

    if {$state} {
        # turn logging on
        package require base32
        package require md5

        # make a date subdir within the log dir
        if {![info exists env(RAPPTURE_LOG)]} {
            set env(RAPPTURE_LOG) /tmp
        }
        set dir [clock format [clock seconds] -format "%Y-%m-%d"]
        set dir [file join $env(RAPPTURE_LOG) $dir]
        if {![file isdirectory $dir]} {
            puts stderr "WARNING: log directory \"$dir\" doesn't exist"
            return
        }

        # generate a unique random file name for the log
        set app [Rappture::Tool::resources -appname]
        if {$app eq ""} {
            error "app name not set before logging -- init logging after initializing tool resources"
        }

        for {set ntries 0} {$ntries < 5} {incr ntries} {
            set unique [string map {= _} [base32::encode [md5::md5 "$tcl_platform(user):$app:[clock seconds]:[clock clicks]"]]]

            set fileName [file join $dir $unique.log]
            if {![file exists $fileName]} {
                break
            }
        }
        if {[file exists $fileName]} {
            error "can't seem to create a unique log file name in $dir"
        }

        # open the log file
        set fid [open $fileName w]

        # set the buffer size low so we don't lose much output if the
        # program suddenly terminates
        fconfigure $fid -buffersize 1024

        # set permissions on the file so that it's not readable
        # NOTE: middleware should do this automatically
        ##file attributes $fileName -permissions 0400

        # logging is now turned on
        set enabled 1

        # create the logging proc -- this is faster than checking "enabled"
        proc ::Rappture::Logger::log {event args} {
            variable fid
            # limit the side of log messages (which could be really large)
            set shortArgs ""
            foreach str $args {
                if {[string length $str] > 255} {
                    set str "[string range $str 0 255]..."
                }
                lappend shortArgs $str
            }
            catch {
              set t [clock seconds]
              set tstr [clock format $t -format "%T"]
              puts $fid "$t = $tstr>> $event $shortArgs"
            }
        }

        # save out some info about the user and the session
        puts $fid "# Application: $app"
        puts $fid "# User: $tcl_platform(user)"
        if {[info exists env(SESSION)]} {
            puts $fid "# Session: $env(SESSION)"
        }
        puts $fid "# Date: [clock format [clock seconds]]"
        Rappture::Logger::log started

    } else {
        # turn logging off
        if {$fid ne ""} {
            catch {close $fid}
            set fid ""
        }
        set enabled 0

        # null out the logging proc -- this is faster than checking "enabled"
        proc ::Rappture::Logger::log {event args} { # do nothing }
    }

    # catch signals that kill the program and clean up logging
    package require Rappture
    Rappture::signal SIGHUP RapptureLogger ::Rappture::Logger::cleanup
    Rappture::signal SIGINT RapptureLogger ::Rappture::Logger::cleanup
    Rappture::signal SIGQUIT RapptureLogger ::Rappture::Logger::cleanup
    Rappture::signal SIGTERM RapptureLogger ::Rappture::Logger::cleanup
    Rappture::signal SIGKILL RapptureLogger ::Rappture::Logger::cleanup
}

# ----------------------------------------------------------------------
# USAGE: log <event> ?<detail> <detail> ...?
#
# Used throughout the Rappture application to log various activities.
# Each <event> is a short name like "tooltip" or "inputChanged" that
# indicates what happened.  All remaining arguments are appended to
# the log as details about what happened or what changed.
# ----------------------------------------------------------------------
proc Rappture::Logger::log {event args} {
    # do nothing by default until turned on by init
}

# ----------------------------------------------------------------------
# USAGE: cleanup
#
# Called when the program receives a signal that would normally kill
# the program.  Flushes the buffer and closes the log file cleanly
# before dying.
# ----------------------------------------------------------------------
proc Rappture::Logger::cleanup {} {
    variable enabled
    variable fid

    if {$enabled && $fid ne ""} {
        log finished
        catch {flush $fid}
        catch {close $fid}
        set fid ""
        set enabled 0
    }
    after idle exit
}
