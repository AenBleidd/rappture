# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: tool - represents an entire tool
#
#  This object represents an entire tool defined by Rappture.
#  Each tool resides in an installation directory with other tool
#  resources (libraries, examples, etc.).  Each tool is defined by
#  its inputs and outputs, which are tied to various widgets in the
#  GUI.  Each tool tracks the inputs, knows when they're changed,
#  and knows how to run itself to produce new results.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2014  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================

itcl::class Rappture::Tool {
    inherit Rappture::ControlOwner

    constructor {xmlobj installdir} {
        Rappture::ControlOwner::constructor ""
    } { # defined below }

    destructor { # defined below }

    public method installdir {} {
        return [$_task installdir]
    }
    public method run {args} {
        sync  ;# sync all widget values to XML

        foreach {status result} [eval $_task run $args] break
        if {$status == 0} {
            # move good results to the data/results directory
            $_task save $result
        }

        return [list $status $result]
    }
    public method abort {} {
        $_task abort
    }
    public method reset {} {
        $_task reset
    }

    private variable _task ""  ;# underlying task for the tool

    # global resources for this tool session (from task)
    public proc resources {{option ""}} {
        eval ::Rappture::Task::resources $option
    }
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Tool::constructor {xmlobj installdir} {
    if {![Rappture::library isvalid $xmlobj]} {
        error "bad value \"$xmlobj\": should be Rappture::Library"
    }

    set _task [Rappture::Task ::#auto $xmlobj $installdir \
        -logger ::Rappture::Logger::log]

    # save a reference to the tool XML in the ControlOwner
    set _xmlobj $xmlobj
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Tool::destructor {} {
    itcl::delete object $_task
}
