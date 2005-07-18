# ----------------------------------------------------------------------
#  COMPONENT: exec - simple way to exec with streaming output
#
#  This utility makes it easy to exec another tool and get streaming
#  output from stdout and stderr.  Any stderr messages are specially
#  encoded with the =RAPPTURE-ERROR=> tag, so they show up in red
#  in the execution output.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005
#  Purdue Research Foundation, West Lafayette, IN
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

    set execout ""
    eval blt::bgexec control \
        -onoutput {{::Rappture::_exec_out stdout}} \
        -onerror {{::Rappture::_exec_out stderr}} \
        $args

    return $execout
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
        set newmesg ""
        if {[string length $execout] > 0
              && [string index $execout end] != "\n"} {
            set newmesg "\n"
        }
        foreach line [split $message \n] {
            append newmesg "=RAPPTURE-ERROR=>$line\n"
        }
        set message [string trimright $newmesg \n]
    }

    puts $message
    append execout $message "\n"
}
