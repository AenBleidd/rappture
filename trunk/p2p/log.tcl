# ----------------------------------------------------------------------
#  LIBRARY: logging routines
# ----------------------------------------------------------------------
#  Michael McLennan (mmclennan@purdue.edu)
# ======================================================================
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
# ----------------------------------------------------------------------
#  USAGE: log channel <channel> ?on|off?
#  USAGE: log <channel> <message>
#  USAGE: log retrieve
#  USAGE: log clear
#
#  Used to handle log messages within this package.  Typical usage
#  sequence starts with one or more "log channel" statements, which
#  define a series of message channels and whether they are on or off.
#  Then, as "log <channel>" statements are encountered within the code,
#  they are either passed along to the log file or ignored.  At any
#  point, the contents of the log file can be retrieved by calling
#  "log retrieve", and it can be cleared by calling "log clear".
# ----------------------------------------------------------------------

set log_options(tmpdir) "/tmp"
set log_options(logfile) ""
set log_options(logfid) ""

proc log {option args} {
    global log_options log_channels

    switch -- $option {
        channel {
            # ----------------------------------------------------------
            #  HANDLE: log channel
            # ----------------------------------------------------------
            if {[llength $args] > 2} {
                error "wrong # args: should be \"log channel name ?state?\""
            }
            set channel [lindex $args 0]
            if {[llength $args] == 1} {
                if {[info exists log_channels($channel)]} {
                    return $log_channels($channel)
                }
                return "off"
            }

            set state [lindex $args 1]
            if {![string is boolean $state]} {
                error "bad value \"$state\": should be on/off"
            }
            set log_channels($channel) $state

            # make sure the log file is open and ready
            if {"" == $log_options(logfid)} {
                log clear
            }
            return $log_channels($channel)
        }
        retrieve {
            # ----------------------------------------------------------
            #  HANDLE: log retrieve
            # ----------------------------------------------------------
            if {[llength $args] != 0} {
                error "wrong # args: should be \"log retrieve\""
            }
            if {[string length $log_options(logfile)] == 0} {
                return ""
            }
            if {[catch {
                close $log_options(logfid)
                set fid [open $log_options(logfile) r]
                set info [read $fid]
                close $fid
                set log_options(logfid) [open $log_options(logfile) a]
              } result]} {
                return "ERROR: $result"
            }
            return $info
        }
        clear {
            # ----------------------------------------------------------
            #  HANDLE: log clear
            # ----------------------------------------------------------
            if {[llength $args] != 0} {
                error "wrong # args: should be \"log clear\""
            }

            # close any existing file
            if {"" != $log_options(logfid)} {
                catch {close $log_options(logfid)}
                set log_options(logfid) ""
            }

            # don't have a log file name yet?  then look for a good name
            if {[string length $log_options(logfile)] == 0} {
                set counter 0
                while {[incr counter] < 500} {
                    set fname [file join $log_options(tmpdir) log[pid].$counter]
                    if {[catch {
                        set fid [open $fname w]; close $fid
                      } result] == 0} {
                        # success! use this name
                        set log_options(logfile) $fname
                        break
                    }
                }
            }
            if {[string length $log_options(logfile)] == 0} {
                error "couldn't open log file in directory $log_options(tmpdir)"
            }
            set log_options(logfid) [open $log_options(logfile) w]
            return ""
        }
        default {
            # ----------------------------------------------------------
            #  HANDLE: log <channel> <message>
            # ----------------------------------------------------------
            if {![info exists log_channels($option)] &&
                  [lsearch {channel retrieve clear} $option] < 0} {
                error "bad option \"$option\": should be a channel defined by \"log channel\" or one of channel, retrieve, or clear"
            }
            if {$log_channels($option)} {
                if {[llength $args] != 1} {
                    error "wrong # args: should be \"log chan message\""
                }
                set date [clock format [clock seconds] -format "%m/%d/%Y %H:%M:%S"]
                puts $log_options(logfid) "$date [lindex $args 0]"
                flush $log_options(logfid)
            }
        }
    }
}
