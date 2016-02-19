# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: task - represents the executable part of a tool
#
#  This object is an executable version of a Rappture xml file.
#  A tool is a task plus its graphical user interface.  Each task
#  resides in an installation directory with other tool resources
#  (libraries, examples, etc.).  Each task is defined by its inputs
#  and outputs, and understands the context in which it executes
#  (via exec, submit, mx, etc.).
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2014  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require BLT

itcl::class Rappture::Task {
    public variable logger ""
    public variable jobstats Rappture::Task::MiddlewareTime
    public variable resultdir "@default"

    constructor {xmlobj installdir args} { # defined below }
    destructor { # defined below }

    public method installdir {} { return $_installdir }

    public method run {args}
    public method get_uq {args}
    public method abort {}
    public method reset {}
    public method xml {args}
    public method save {xmlobj {name ""}}

    protected method _mkdir {dir}
    protected method _output {data}
    protected method _log {args}
    protected method _build_submit_cmd {cmd tfile params_file}
    protected method _get_params {varlist uq_type uq_args}

    private variable _xmlobj ""      ;# XML object with inputs/outputs
    private variable _origxml ""     ;# copy of original XML (for reset)
    private variable _lastrun ""     ;# name of last run file
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

    # default method for -jobstats control
    public proc MiddlewareTime {args}
}

# must use this name -- plugs into Rappture::resources::load
proc task_init_resources {} {
    Rappture::resources::register \
        application_name  Rappture::Task::setAppName \
        application_id    Rappture::Task::setAppId \
        hub_name          Rappture::Task::setHubName \
        hub_url           Rappture::Task::setHubURL \
        session_token     Rappture::Task::setSession \
        job_protocol      Rappture::Task::setJobPrt \
        results_directory Rappture::Task::setResultDir
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Task::constructor {xmlobj installdir args} {
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
itcl::body Rappture::Task::destructor {} {
    itcl::delete object $_origxml
}

# ----------------------------------------------------------------------
# USAGE: resources ?-option?
#
# Clients use this to query information about the tool.
# ----------------------------------------------------------------------
itcl::body Rappture::Task::resources {{option ""}} {
    if {$option == ""} {
        return [array get _resources]
    }
    if {[info exists _resources($option)]} {
        return $_resources($option)
    }
    return ""
}

itcl::body Rappture::Task::get_uq {args} {
    foreach {path val} $args {
        if {$path == "-uq_type"} {
            set uq_type $val
        } elseif {$path == "-uq_args"} {
            set uq_args $val
        }
    }
    #set varlist [$_xmlobj uq_get_vars]
    foreach {varlist num} [$_xmlobj uq_get_vars] break
    return [Rappture::UQ ::#auto $varlist $num $uq_type $uq_args]
}

# ----------------------------------------------------------------------
# USAGE: run ?<path1> <value1> <path2> <value2> ...? ?-output <callbk>?
#
# This method causes the tool to run.  A "driver.xml" file is created
# as the input for the run.  That file is fed to the executable
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
itcl::body Rappture::Task::run {args} {
    global env errorInfo

    #
    # Make sure that we save the proper application name.
    # Actually, the best place to get this information is
    # straight from the "installtool" script, but just in
    # case we have an older tool, we should insert the
    # tool name from the resources config file.
    #
    if {[info exists _resources(-appname)]
          && $_resources(-appname) ne ""
          && [$_xmlobj get tool.name] eq ""} {
        $_xmlobj put tool.name $_resources(-appname)
    }

    # if there are any args, use them to override parameters
    set _outputcb ""
    set uq_type ""
    foreach {path val} $args {
        if {$path == "-output"} {
            set _outputcb $val
        } elseif {$path == "-uq_type"} {
            set uq_type $val
        } elseif {$path == "-uq_args"} {
            set uq_args $val
        } else {
            $_xmlobj put $path.current $val
        }
    }

    foreach item {control output error} { set job($item) "" }

    # Set limits for cpu time
    set limit [$_xmlobj get tool.limits.cputime]
    if { $limit == "unlimited" } {
        set limit 43200;                # 12 hours
    } else {
        if { [scan $limit "%d" dum] != 1 } {
            set limit 14400;            # 4 hours by default
        } elseif { $limit > 43200 } {
            set limit 43200;            # limit to 12 hrs.
        } elseif { $limit < 10 } {
            set limit 10;               # lower bound is 10 seconds.
        }
    }
    Rappture::rlimit set cputime $limit

    # write out the driver.xml file for the tool
    set file "driver[pid].xml"
    set status [catch {
        set fid [open $file w]
        puts $fid "<?xml version=\"1.0\"?>"
        puts $fid [$_xmlobj xml]
        close $fid
    } result]

    if {$uq_type != ""} {
        # Copy xml into a new file
        set tfile "template[pid].xml"
        set fid [open $tfile w]
        puts $fid "<?xml version=\"1.0\"?>"
        puts $fid [$_xmlobj xml]
        close $fid

        # Return a list of the UQ variables and their PDFs.
        # Also turns $tfile into a template file.
        set uq_varlist [lindex [$_xmlobj uq_get_vars $tfile] 0]
    }


    # execute the tool using the path from the tool description
    if {$status == 0} {
        set cmd [$_xmlobj get tool.command]
        regsub -all @tool $cmd $_installdir cmd
        set cmd [string trimleft $cmd " "]

        if { $cmd == "" } {
            puts stderr "cmd is empty"
            return [list 1 "Command is empty.\n\nThere is no command specified by\n\n <command>\n </command>\n\nin the tool.xml file."]
        }

        if {$uq_type == ""} {
            regsub -all @driver $cmd $file cmd

            switch -glob -- [resources -jobprotocol] {
                "submit*" {
                    # if job_protocol is "submit", then use use submit command
                    set cmd "submit --local $cmd"
                }
                "mx" {
                    # metachory submission
                    set cmd "mx $cmd"
                }
                "exec" {
                    # default -- nothing special
                }
            }
        } else {
            set params_file [_get_params $uq_varlist $uq_type $uq_args]
            set cmd [_build_submit_cmd $cmd $tfile $params_file]
            file delete -force puq
        }

        $_xmlobj put tool.execute $cmd

        # starting job...
        set _lastrun ""
        _log run started
        Rappture::rusage mark

        if {0 == [string compare -nocase -length 5 $cmd "ECHO "] } {
            set status 0;
            set job(output) [string range $cmd 5 end]
        } else {
            set status [catch {
                set ::Rappture::Task::job(control) ""
                eval blt::bgexec \
                ::Rappture::Task::job(control) \
                -keepnewline yes \
                -killsignal SIGTERM \
                -onoutput [list [itcl::code $this _output]] \
                -output ::Rappture::Task::job(output) \
                -error ::Rappture::Task::job(error) \
                $cmd
            } result]

            if { $status != 0 } {
                # We're here because the exec-ed program failed
                set logmesg $result
                if { $::Rappture::Task::job(control) ne "" } {
                    foreach { token pid code mesg } \
                    $::Rappture::Task::job(control) break
                    if { $token == "EXITED" } {
                       # This means that the program exited normally but
                       # returned a non-zero exitcode.  Consider this an
                       # invalid result from the program.  Append the stderr
                       # from the program to the message.
                       set logmesg "Program finished: exit code is $code"
                       set result "$logmesg\n\n$::Rappture::Task::job(error)"
                    } elseif { $token == "abort" }  {
                        # The user pressed the abort button.
                        set logmesg "Program terminated by user."
                        set result "$logmesg\n\n$::Rappture::Task::job(output)"
                    } else {
                        # Abnormal termination
                        set logmesg "Abnormal program termination: $mesg"
                        set result "$logmesg\n\n$::Rappture::Task::job(output)"
                    }
                }
                _log run failed [list $logmesg]
                return [list $status $result]
            }
        }
        # ...job is finished
        array set times [Rappture::rusage measure]

        if {[resources -jobprotocol] ne "submit"} {
            set id [$_xmlobj get tool.id]
            set vers [$_xmlobj get tool.version.application.revision]
            set simulation simulation
            if { $id ne "" && $vers ne "" } {
                set pid [pid]
                set simulation ${pid}_${id}_r${vers}
            }

            # need to save job info? then invoke the callback
            if {[string length $jobstats] > 0} {
                uplevel #0 $jobstats [list job [incr jobnum] \
                event $simulation start $times(start) \
                walltime $times(walltime) cputime $times(cputime) \
                status $status]
            }

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

                set details ""
                foreach key {job event start walltime cputime status} {
                    # add required keys in a particular order
                    lappend details $key $data($key)
                    unset data($key)
                }
                foreach key [array names data] {
                    # add anything else that the client gave -- venue, etc.
                    lappend details $key $data($key)
                }

                if {[string length $jobstats] > 0} {
                    uplevel #0 $jobstats $details
                }

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
        # file delete -force -- $file
    }

    # see if the job was aborted
    if {[regexp {^KILLED} $job(control)]} {
        _log run aborted
        return [list 0 "ABORT"]
    }

    #
    # If successful, return the output, which should include
    # a reference to the run.xml file containing results.
    #

    if {$status == 0} {
        set result [string trim $job(output)]

        if {$uq_type != ""} {
            # UQ. Collect data from all jobs and put it in one xml run file.
            file delete -force -- run_uq.xml
            if {[catch {exec puq_analyze.py puq_[pid].hdf5} res]} {
                set fp [open "uq_debug.err" r]
                set rdata [read $fp]
                close $fp
                puts "PUQ analysis failed: $res\n$rdata"
                error "UQ analysis failed: $res\n$rdata"
            } else {
                append result "\n" $res
            }
        }
        if {[regexp {=RAPPTURE-RUN=>([^\n]+)} $result match file]} {
            set _lastrun $file

            set status [catch {Rappture::library $file} result]
            if {$status == 0} {
                # add cputime info to run.xml file
                $result put output.walltime $times(walltime)
                $result put output.cputime $times(cputime)
                if {[info exists env(SESSION)]} {
                    $result put output.session $env(SESSION)
                }
            } else {
                global errorInfo
                set result "$result\n$errorInfo"
            }

            file delete -force -- $file
        } else {
            set status 1
            set result "Can't find result file in output.\nDid you call Rappture
::result in your simulator?"
        }
    } elseif {$job(output) ne "" || $job(error) ne ""} {
        set result [string trim "$job(output)\n$job(error)"]
    }

    # log final status for the run
    if {$status == 0} {
        _log run finished
    } else {
        _log run failed [list $result]
    }

    return [list $status $result]
}

# ----------------------------------------------------------------------
#  Turn the command string from tool.xml into the proper syntax to use
#  with a submit parameter sweep with a temlate file.  Proper quoting
# of the template file is necessary to prevent submit from being too smart
# and converting it to a full pathname.
# ----------------------------------------------------------------------
itcl::body Rappture::Task::_build_submit_cmd {cmd tfile params_file} {
    set quote_next 0
    set newcmd "submit --progress submit --runName=puq -l -i @:$tfile -d $params_file"
    set cmds [split $cmd " "]
    for {set i 0} {$i < [llength $cmds]} {incr i} {
        set arg [lindex $cmds $i]
        if {$quote_next == 1} {
            set nc [string range $arg 0 0]
            if {$nc != "\""} {
                set arg "\"\\\"$arg\\\"\""
            }
        }
        if {$arg == "--eval"} {
            set quote_next 1
        } else {
            set quote_next 0
        }
        if {$arg == "@driver"} {
            set arg "\"\\\"$tfile\\\"\""
        }
        append newcmd " " $arg
    }
    regsub -all @driver $newcmd $tfile newcmd
    return $newcmd
}

# ----------------------------------------------------------------------
# USAGE: _mkdir <directory>
#
# Used internally to create the <directory> in the file system.
# The parent directory is also created, as needed.
# ----------------------------------------------------------------------
itcl::body Rappture::Task::_mkdir {dir} {
    set parent [file dirname $dir]
    if {$parent ne "." && $parent ne "/"} {
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
itcl::body Rappture::Task::abort {} {
    _log run abort
    set job(control) "abort"
}

# ----------------------------------------------------------------------
# USAGE: reset
#
# Resets all input values to their defaults.  Sometimes used just
# before a run to reset to a clean state.
# ----------------------------------------------------------------------
itcl::body Rappture::Task::reset {} {
    $_xmlobj copy "" from $_origxml ""
    foreach path [Rappture::entities -as path $_xmlobj input] {
        if {[$_xmlobj element -as type $path.default] ne ""} {
            set defval [$_xmlobj get $path.default]
            $_xmlobj put $path.current $defval
        }
    }
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
itcl::body Rappture::Task::xml {args} {
    if {"object" == $args} {
        return $_xmlobj
    }
    return [eval $_xmlobj $args]
}

# ----------------------------------------------------------------------
# USAGE: save <xmlobj> ?<filename>?
#
# Used by clients to save the contents of an <xmlobj> representing
# a run out to the given file.  If <filename> is not specified, then
# it uses the -resultsdir and other settings to do what Rappture
# would normally do with the output.
# ----------------------------------------------------------------------
itcl::body Rappture::Task::save {xmlobj {filename ""}} {
    if {$filename eq ""} {
        # if there's a results_directory defined in the resources
        # file, then move the run.xml file there for storage
        set rdir ""
        if {$resultdir eq "@default"} {
            if {[info exists _resources(-resultdir)]} {
                set rdir $_resources(-resultdir)
            } else {
                set rdir "."
            }
        } elseif {$resultdir ne ""} {
            set rdir $resultdir
        }

        # use the runfile name generated by the last run
        if {$_lastrun ne ""} {
            set filename [file join $rdir $_lastrun]
        } else {
            set filename [file join $rdir run.xml]
        }
    }

    # add any last-minute metadata
    $xmlobj put output.time [clock format [clock seconds]]

    $xmlobj put tool.version.rappture.version $::Rappture::version
    $xmlobj put tool.version.rappture.revision $::Rappture::build

    if {[info exists ::tcl_platform(user)]} {
        $xmlobj put output.user $::tcl_platform(user)
    }

    # save the output
    set rdir [file dirname $filename]
    if {![file exists $rdir]} {
        _mkdir $rdir
    }

    set fid [open $filename w]
    puts $fid "<?xml version=\"1.0\"?>"
    puts $fid [$xmlobj xml]
    close $fid

    _log output saved in $filename
}

# ----------------------------------------------------------------------
# USAGE: _output <data>
#
# Used internally to send each bit of output <data> coming from the
# tool onto the caller, so the user can see progress.
# ----------------------------------------------------------------------
itcl::body Rappture::Task::_output {data} {
    if {[string length $_outputcb] > 0} {
        uplevel #0 $_outputcb [list $data]
    }
}

# ----------------------------------------------------------------------
# USAGE: _log <cmd> <arg> <arg> ...
#
# Used internally to log interesting events during the run.  If the
# -logger option is set (to Rappture::Logger::log, or something like
# that), then the arguments to this method are passed along to the
# logger and written out to a log file.  Logging is off by default,
# so this method does nothing unless -logger is set.
# ----------------------------------------------------------------------
itcl::body Rappture::Task::_log {args} {
    if {[string length $logger] > 0} {
        uplevel #0 $logger [list $args]
    }
}

# ----------------------------------------------------------------------
# USAGE: MiddlewareTime <key> <value> ...
#
# Used as the default method for reporting job status information.
# Implements the old HUBzero method of reporting job status info to
# stderr, which can then be picked up by the tool session container.
# Most tools use the "submit" command, which talks directly to a
# database to log job information, so this isn't really needed.  But
# it doesn't hurt to have this and it can be useful in some cases.
# ----------------------------------------------------------------------
itcl::body Rappture::Task::MiddlewareTime {args} {
    set line "MiddlewareTime:"
    foreach {key val} $args {
        append line " $key=$val"
    }
    puts stderr $line
}


#
# Send the list of parameters to a python program so it can call PUQ
# and get a CSV file containing the parameter values to use for the runs.
itcl::body Rappture::Task::_get_params {varlist uq_type uq_args} {
    set pid [pid]
    # puts "get_params.py $pid $varlist $uq_type $uq_args"
    if {[catch {exec get_params.py $pid $varlist $uq_type $uq_args}]} {
        set fp [open "uq_debug.err" r]
        set rdata [read $fp]
        close $fp
        puts "get_params.py failed: $rdata"
        error "get_params.py: $rdata"
    }
    return params[pid].csv
}
