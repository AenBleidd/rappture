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
#  Copyright (c) 2004-2005
#  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require BLT

itcl::class Rappture::Tool {
    public variable analyzer ""

    constructor {xmlobj installdir args} { # defined below }

    public method installdir {} { return $_installdir }
    public method xml {args}

    public method load {xmlobj}
    public method run {args}
    public method abort {}

    public method widgetfor {path {widget ""}}
    public method changed {path}
    public method sync {}
    public method tool {}

    private variable _xmlobj ""      ;# XML overall <run> object
    private variable _installdir ""  ;# installation directory for this tool
    private variable _path2widget    ;# maps path => widget on this page

    private common job               ;# array var used for blt::bgexec jobs
}
                                                                                
# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Tool::constructor {xmlobj installdir args} {
    if {![Rappture::library isvalid $xmlobj]} {
        error "bad value \"$xmlobj\": should be Rappture::Library"
    }
    set _xmlobj $xmlobj

    if {![file exists $installdir]} {
        error "directory \"$installdir\" doesn't exist"
    }
    set _installdir $installdir

    eval configure $args
}

# ----------------------------------------------------------------------
# USAGE: xml <subcommand> ?<arg> <arg> ...?
# USAGE: xml object
#
# Used by clients to manipulate the underlying XML data for this
# tool.  The <subcommand> can be any operation supported by a
# Rappture::library object.  Clients can also request the XML object
# directly by using the "object" subcommand.
# ----------------------------------------------------------------------
itcl::body Rappture::Tool::xml {args} {
    if {"object" == $args} {
        return $_xmlobj
    }
    return [eval $_xmlobj $args]
}

# ----------------------------------------------------------------------
# USAGE: run ?<path1> <value1> <path2> <value2> ...?
#
# This method causes the tool to run.  All widgets are synchronized
# to the current XML representation, and a "driver.xml" file is
# created as the input for the run.  That file is fed to the tool
# according to the <tool><command> string, and the job is executed.
#
# Returns a list of the form {status result}, where status is an
# integer status code (0=success) and result is the output from the
# simulator.  Successful output is something like {0 run1293921.xml},
# where 0=success and run1293921.xml is the name of the file containing
# results.
# ----------------------------------------------------------------------
itcl::body Rappture::Tool::run {args} {
    global errorInfo

    # sync all widgets to the XML tree
    sync

    # if there are any args, use them to override parameters
    foreach {path val} $args {
        $_xmlobj put $path.current $val
    }

    foreach item {control output error} { set job($item) "" }

    # write out the driver.xml file for the tool
    set file "driver[pid].xml"
    set status [catch {
        set fid [open $file w]
        puts $fid "<?xml version=\"1.0\"?>"
        puts $fid [$_xmlobj xml]
        close $fid
    } result]

    # execute the tool using the path from the tool description
    if {$status == 0} {
        set cmd [$_xmlobj get tool.command]
        regsub -all @tool $cmd $_installdir cmd
        regsub -all @driver $cmd $file cmd

        set status [catch {eval blt::bgexec \
            ::Rappture::Tool::job(control) \
            -output ::Rappture::Tool::job(output) \
            -error ::Rappture::Tool::job(error) $cmd} result]
    } else {
        set job(error) "$result\n$errorInfo"
    }
    if {$status == 0} {
        file delete -force -- $file
    }

    # see if the job was aborted
    if {[regexp {^KILLED} $job(control)]} {
        return [list 0 "ABORT"]
    }

    #
    # If successful, return the output, which should include
    # a reference to the run.xml file containing results.
    #
    if {$status == 0} {
        set file [string trim $job(output)]
        return [list $status $file]
    } elseif {"" != $job(output) || "" != $job(error)} {
        return [list $status [string trim "$job(output)\n$job(error)"]]
    }
    return [list $status $result]
}

# ----------------------------------------------------------------------
# USAGE: abort
#
# Clients use this during a "run" to abort the current job.
# Kills the job and forces the "run" method to return.
# ----------------------------------------------------------------------
itcl::body Rappture::Tool::abort {} {
    set job(control) "abort"
}

# ----------------------------------------------------------------------
# USAGE: widgetfor <path> ?<widget>?
#
# Used by embedded widgets such as a Controls panel to register the
# various controls associated with this page.  That way, this Tool
# knows what widgets to look at when syncing itself to the underlying
# XML data.
# ----------------------------------------------------------------------
itcl::body Rappture::Tool::widgetfor {path {widget ""}} {
    # if this is a query operation, then look for the path
    if {"" == $widget} {
        if {[info exists _path2widget($path)]} {
            return $_path2widget($path)
        }
        return ""
    }

    # otherwise, associate the path with the given widget
    if {[info exists _path2widget($path)]} {
        error "$path already associated with widget $_path2widget($path)"
    }
    set _path2widget($path) $widget
}

# ----------------------------------------------------------------------
# USAGE: load <xmlobj>
#
# Loads the contents of a Rappture <xmlobj> into the controls
# associated with this tool.
# ----------------------------------------------------------------------
itcl::body Rappture::Tool::load {newobj} {
    if {![Rappture::library isvalid $newobj]} {
        error "\"$newobj\" is not a Rappture::library"
    }

    foreach path [array names _path2widget] {
        if {"" != [$newobj element -as type $path.current]} {
            set val [$newobj get $path.current]
            if {[string length $val] > 0
                  || [llength [$newobj children $path.current]] == 0} {
                $_path2widget($path) value $val
            } else {
                set obj [$newobj element -as object $path.current]
                $_path2widget($path) value $obj
            }
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: changed <path>
#
# Invoked automatically by the various widgets associated with this
# tool whenever their value changes.  If this tool has a -analyzer,
# then it is notified that input has changed, so it can reset itself
# for a new analysis.
# ----------------------------------------------------------------------
itcl::body Rappture::Tool::changed {path} {
    if {"" != $analyzer} {
        $analyzer reset
    }
}

# ----------------------------------------------------------------------
# USAGE: sync
#
# Used by descendents such as a Controls panel to register the
# various controls associated with this page.  That way, this Tool
# knows what widgets to look at when syncing itself to the underlying
# XML data.
# ----------------------------------------------------------------------
itcl::body Rappture::Tool::sync {} {
    foreach path [array names _path2widget] {
        $_xmlobj put $path.current [$_path2widget($path) value]
    }
}

# ----------------------------------------------------------------------
# USAGE: tool
#
# Clients use this to figure out which tool is associated with
# this object.  Since this is a tool, it returns itself.
# ----------------------------------------------------------------------
itcl::body Rappture::Tool::tool {} {
    return $this
}
