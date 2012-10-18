
# ----------------------------------------------------------------------
#  COMPONENT: exec - simple way to exec with streaming output
#
#  This utility makes it easy to exec another tool and get streaming
#  output from stdout and stderr.  Any stderr messages are specially
#  encoded with the =RAPPTURE-ERROR=> tag, so they show up in red
#  in the execution output.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require BLT

namespace eval Rappture { # forward declaration }

# ----------------------------------------------------------------------
# USAGE: exec <arg> <arg>...
#
# This utility acts like the standard Tcl "exec" command, but it also
# streams stdout and stderr messages while the command is executing,
# instead of waiting till the end.  Any error messages are prefixed
# with the special =RAPPTURE-ERROR=> tag, so they are recognized as
# errors within Rappture.
# ----------------------------------------------------------------------
proc Rappture::exec {args} {
    variable execout
    variable execctl

    set execout(output) ""
    set execout(channel) ""
    set execout(extra) ""
    set execctl ""

    set status [catch {eval blt::bgexec ::Rappture::execctl \
        -keepnewline yes \
        -killsignal SIGTERM \
        -onoutput {{::Rappture::_exec_out stdout}} \
        -onerror {{::Rappture::_exec_out stderr}} \
        $args} result]

    # add any extra stuff pending from the last stdout/stderr change
    append execout(output) $execout(extra)

    if { $status != 0 } {
	# We're here because the exec-ed program failed 
	if { $execctl != "" } {
	    foreach { token pid code mesg } $execctl break
	    if { $token == "EXITED" } {
		# This means that the program exited normally but
		# returned a non-zero exitcode.  Consider this an
		# invalid result from the program.  Append the stderr
		# from the program to the message.
		set result \
		    "Program finished: exit code is $code\n\n"
		append result $execout(error)
	    } elseif { $token == "abort" }  {
		# The user pressed the abort button.
		set result "Program terminated by user.\n\n"
		append result $execout(output)
	    } else {
		# Abnormal termination
		set result "Abnormal program termination: $mesg\n\n"
		append result $execout(output)
	    }
	}
        error $result
    }
    return $execout(output)
}

# ----------------------------------------------------------------------
# USAGE: _exec_out <channel> <message>
#
# Called automatically whenever output comes in from the Rappture::exec
# utility.  Streams the output to stdout and adds it to the "execout"
# variable, so it can be returned as one large string at the end of
# the exec.
# ----------------------------------------------------------------------
proc Rappture::_exec_out {channel message} {
    variable execout

    #
    # If this message is being written to the stderr channel, then
    # add the =RAPPTURE-ERROR=> prefix to each line.
    #
    if {$channel == "stderr"} {
	append execout(error) $message
        set newmesg ""
        foreach line [split $message \n] {
            append newmesg "=RAPPTURE-ERROR=>$line\n"
        }
        set message $newmesg
    }
    #
    # If this message is coming in on the same channel as the
    # last, then fine, add it on.  But if it's coming in on a
    # different channel, we must make sure that we're at a good
    # breakpoint.  If there's not a line break at the end of the
    # current output, then add the extra stuff onto a buffer
    # that we will merge in later once we get to a good point.
    #
    if {$execout(channel) == ""} {
        set execout(channel) $channel
    }

    set ready [expr {[string length $execout(output)] == 0
        || [string index $execout(output) end] == "\n"}]

    if {$channel != $execout(channel)} {
        if {$ready} {
            # changing channels...
            if {[string length $execout(extra)] > 0} {
                # any extra stuff on this new channel? put it out now
                puts -nonewline $execout(extra)
                append execout(output) $execout(extra)
                set execout(extra) ""
            }
            puts -nonewline $message
            append execout(output) $message
            set execout(channel) $channel
        } else {
            # not ready to change channels -- keep this for later
            append execout(extra) $message
        }
    } else {
        # no need to change channels -- keep printing
        puts -nonewline $message
        append execout(output) $message
    }
}
