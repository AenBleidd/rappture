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
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require BLT

itcl::class Rappture::Tool {
    inherit Rappture::ControlOwner

    constructor {xmlobj installdir args} {
        Rappture::ControlOwner::constructor ""
    } { # defined below }

    destructor { # defined below }

    public method installdir {} { return $_installdir }

    public method run {args}
    public method abort {}
    public method reset {}

    protected method _mkdir {dir}
    protected method _output {data}

    private variable _origxml ""     ;# copy of original XML (for reset)
    private variable _installdir ""  ;# installation directory for this tool
    private variable _outputcb ""    ;# callback for tool output
    private common job               ;# array var used for blt::bgexec jobs
    private common jobnum 0          ;# counter for unique job number

    # get global resources for this tool session
    public proc resources {{option ""}}

    public common _resources
    public proc setAppName {name}   { set _resources(-appname) $name }
    public proc setHubName {name}   { set _resources(-hubname) $name }
    public proc setHubURL {name}    { set _resources(-huburl) $name }
    public proc setSession {name}   { set _resources(-session) $name }
    public proc setJobPrt {name}    { set _resources(-jobprotocol) $name }
    public proc setResultDir {name} { set _resources(-resultdir) $name }
}

# must use this name -- plugs into Rappture::resources::load
proc tool_init_resources {} {
    Rappture::resources::register \
        application_name  Rappture::Tool::setAppName \
        application_id    Rappture::Tool::setAppId \
        hub_name          Rappture::Tool::setHubName \
        hub_url           Rappture::Tool::setHubURL \
        session_token     Rappture::Tool::setSession \
        job_protocol      Rappture::Tool::setJobPrt \
        results_directory Rappture::Tool::setResultDir
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Tool::constructor {xmlobj installdir args} {
    if {![Rappture::library isvalid $xmlobj]} {
        error "bad value \"$xmlobj\": should be Rappture::Library"
    }
    set _xmlobj $xmlobj

    # stash a copy of the original XML for later "reset" operations
    set _origxml [Rappture::LibraryObj ::#auto "<?xml version=\"1.0\"?><run/>"]
    $_origxml copy "" from $_xmlobj ""

    if {![file exists $installdir]} {
        error "directory \"$installdir\" doesn't exist"
    }
    set _installdir $installdir

    eval configure $args
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Tool::destructor {} {
    itcl::delete object $_origxml
}

# ----------------------------------------------------------------------
# USAGE: resources ?-option?
#
# Clients use this to query information about the tool.
# ----------------------------------------------------------------------
itcl::body Rappture::Tool::resources {{option ""}} {
    if {$option == ""} {
        return [array get _resources]
    }
    if {[info exists _resources($option)]} {
        return $_resources($option)
    }
    return ""
}

# ----------------------------------------------------------------------
# USAGE: run ?<path1> <value1> <path2> <value2> ...? ?-output <callbk>?
#
# This method causes the tool to run.  All widgets are synchronized
# to the current XML representation, and a "driver.xml" file is
# created as the input for the run.  That file is fed to the tool
# according to the <tool><command> string, and the job is executed.
#
# Any "<path> <value>" arguments are used to override the current
# settings from the GUI.  This is useful, for example, when filling
# in missing simulation results from the analyzer.
#
# If the -output argument is included, then the next arg is a
# callback command for output messages.  Any output that comes in
# while the tool is running is sent back to the caller, so the user
# can see progress running the tool.
#
# Returns a list of the form {status result}, where status is an
# integer status code (0=success) and result is the output from the
# simulator.  Successful output is something like {0 run1293921.xml},
# where 0=success and run1293921.xml is the name of the file containing
# results.
# ----------------------------------------------------------------------
itcl::body Rappture::Tool::run {args} {
    global errorInfo

    #
    # Make sure that we save the proper application name.
    # Actually, the best place to get this information is
    # straight from the "installtool" script, but just in
    # case we have an older tool, we should insert the
    # tool name from the resources config file.
    #
    if {[info exists _resources(-appname)]
          && "" != $_resources(-appname)
          && "" == [$_xmlobj get tool.name]} {
        $_xmlobj put tool.name $_resources(-appname)
    }

    # sync all widgets to the XML tree
    sync

    # if there are any args, use them to override parameters
    set _outputcb ""
    foreach {path val} $args {
        if {$path == "-output"} {
            set _outputcb $val
        } else {
            $_xmlobj put $path.current $val
        }
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

    # set limits for cpu time
    set limit [$_xmlobj get tool.limits.cputime]
    if {"" == $limit || [catch {Rappture::rlimit set cputime $limit}]} {
        Rappture::rlimit set cputime 900  ;# 15 mins by default
    }

    # execute the tool using the path from the tool description
    if {$status == 0} {
        set cmd [$_xmlobj get tool.command]
        regsub -all @tool $cmd $_installdir cmd
        regsub -all @driver $cmd $file cmd
        regsub -all {\\} $cmd {\\\\} cmd
        set cmd [string trimleft $cmd " "]

        # if job_protocol is "submit", then use use submit command
        if {[resources -jobprotocol] == "submit"} {
            set cmd [linsert $cmd 0 submit --local]
        }

        # starting job...
        Rappture::rusage mark

        if {0 == [string compare -nocase -length 5 $cmd "ECHO "] } {
            set status 0;
            set job(output) [string range $cmd 5 end] 
        } else {
            set status [catch {eval blt::bgexec \
                ::Rappture::Tool::job(control) \
                -keepnewline yes \
                -killsignal SIGTERM \
                -onoutput [list [itcl::code $this _output]] \
                -output ::Rappture::Tool::job(output) \
                -error ::Rappture::Tool::job(error) $cmd} result]

            if { $status != 0 } {
                foreach {code pid mesg} $::Rappture::Tool::job(control) break
                if { $code != "EXITED" } {
                    set result "Abnormal program termination \"$code\": $mesg"
                    return [list $status $result]
                }
            }
        }
        # ...job is finished
        array set times [Rappture::rusage measure]

        if {[resources -jobprotocol] != "submit"} {
            set id [$_xmlobj get tool.id]
            set vers [$_xmlobj get tool.version.application.revision]
            set simulation simulation
            if { $id != "" && $vers != "" } {
                set pid [pid]
                set simulation ${pid}_${id}_r${vers}
            }
            puts stderr "MiddlewareTime: job=[incr jobnum] event=$simulation start=$times(start) walltime=$times(walltime) cputime=$times(cputime) status=$status"

            #
            # Scan through stderr channel and look for statements that
            # represent grid jobs that were executed.  The statements
            # look like this:
            #
            # MiddlewareTime: job=1 event=simulation start=3.001094 ...
            #
            set subjobs 0
            while {[regexp -indices {(^|\n)MiddlewareTime:( +[a-z]+=[^ \n]+)+(\n|$)} $job(error) match]} {
                foreach {p0 p1} $match break
                if {[string index $job(error) $p0] == "\n"} { incr p0 }

                catch {unset data}
                array set data {
                    job 1
                    event simulation
                    start 0
                    walltime 0
                    cputime 0
                    status 0
                }
                foreach arg [lrange [string range $job(error) $p0 $p1] 1 end] {
                    foreach {key val} [split $arg =] break
                    set data($key) $val
                }
                set data(job) [expr {$jobnum+$data(job)}]
                set data(event) "subsimulation"
                set data(start) [expr {$times(start)+$data(start)}]

                set stmt "MiddlewareTime:"
                foreach key {job event start walltime cputime status} {
                    # add required keys in a particular order
                    append stmt " $key=$data($key)"
                    unset data($key)
                }
                foreach key [array names data] {
                    # add anything else that the client gave -- venue, etc.
                    append stmt " $key=$data($key)"
                }
                puts stderr $stmt
                incr subjobs

                # done -- remove this statement
                set job(error) [string replace $job(error) $p0 $p1]
            }
            incr jobnum $subjobs
        }

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
        set result [string trim $job(output)]
        if {[regexp {=RAPPTURE-RUN=>([^\n]+)} $result match file]} {
            set status [catch {Rappture::library $file} result]
            if {$status != 0} {
                global errorInfo
                set result "$result\n$errorInfo"
            }

            # if there's a results_directory defined in the resources
            # file, then move the run.xml file there for storage
            if {[info exists _resources(-resultdir)]
                  && "" != $_resources(-resultdir)} {
                catch {
                    if {![file exists $_resources(-resultdir)]} {
                        _mkdir $_resources(-resultdir)
                    }
                    file rename -force -- $file $_resources(-resultdir)
                }
            }
        } else {
            set status 1
            set result "Can't find result file in output.\nDid you call Rappture
::result in your simulator?"
        }
        return [list $status $result]
    } elseif {"" != $job(output) || "" != $job(error)} {
        return [list $status [string trim "$job(output)\n$job(error)"]]
    }
    return [list $status $result]
}

# ----------------------------------------------------------------------
# USAGE: _mkdir <directory>
#
# Used internally to create the <directory> in the file system.
# The parent directory is also created, as needed.
# ----------------------------------------------------------------------
itcl::body Rappture::Tool::_mkdir {dir} {
    set parent [file dirname $dir]
    if {"." != $parent && "/" != $parent} {
        if {![file exists $parent]} {
            _mkdir $parent
        }
    }
    file mkdir $dir
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
# USAGE: reset
#
# Resets all input values to their defaults.  Sometimes used just
# before a run to reset to a clean state.
# ----------------------------------------------------------------------
itcl::body Rappture::Tool::reset {} {
    $_xmlobj copy "" from $_origxml ""
    foreach path [Rappture::entities -as path $_xmlobj input] {
        if {[$_xmlobj element -as type $path.default] ne ""} {
            set defval [$_xmlobj get $path.default]
            $_xmlobj put $path.current $defval
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _output <data>
#
# Used internally to send each bit of output <data> coming from the
# tool onto the caller, so the user can see progress.
# ----------------------------------------------------------------------
itcl::body Rappture::Tool::_output {data} {
    if {[string length $_outputcb] > 0} {
        uplevel #0 $_outputcb [list $data]
    }
}
