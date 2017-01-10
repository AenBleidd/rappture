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
    private method CheckForCachedRunFile { driverFile }
    private method CollectUQResults {}
    private method ExecuteSimulationCommand { cmd }
    private method GetCommand {}
    private method GetDriverFile {}
    private method GetSignal { signal }
    private method GetSimulationCommand { driverFile }
    private method GetUQErrors {}
    private method GetUQSimulationCommand { driverFile }
    private method GetUQTemplateFile {}
    private method IsCacheable {}
    private method LogCachedSimulationUsage {}
    private method LogSimulationUsage {}
    private method LogSubmittedSimulationUsage {}
    private method SetCpuResourceLimit {}

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

    protected method OnOutput {data}
    protected method Log {args}
    protected method BuildSubmitCommand {cmd tfile params_file}
    protected method GetParamsForUQ {}

    private variable _xmlobj ""      ;# XML object with inputs/outputs
    private variable _origxml ""     ;# copy of original XML (for reset)
    private variable _installdir ""  ;# installation directory for this tool
    private variable _outputcb ""    ;# callback for tool output
    private common jobnum 0          ;# counter for unique job number
    private variable _uq
    
    private variable _job
    
    # get global resources for this tool session
    public proc resources {{option ""}}

    public common _resources
    public proc setAppName {name}    { set _resources(-appname) $name }
    public proc setHubName {name}    { set _resources(-hubname) $name }
    public proc setHubURL {name}     { set _resources(-huburl) $name }
    public proc setSession {name}    { set _resources(-session) $name }
    public proc setJobPrt {name}     { set _resources(-jobprotocol) $name }
    public proc setResultDir {name}  { set _resources(-resultdir) $name }
    public proc setCacheHosts {name} { set _resources(-cachehosts) $name }

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
        results_directory Rappture::Task::setResultDir \
        cache_hosts       Rappture::Task::setCacheHosts
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
    package require http
    package require tls
    http::register https 443 [list ::tls::socket -tls1 1]

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

itcl::body Rappture::Task::GetSignal {code} {
    set signals {
        xxx HUP INT QUIT ILL TRAP ABRT BUS FPE KILL USR1 SEGV
        USR2 PIPE ALRM TERM STKFLT CHLD CONT STOP TSTP TTIN
        TTOU URG XCPU XFSZ VTALRM PROF WINCH POLL PWR SYS
        RTMIN RTMIN+1 RTMIN+2 RTMIN+3 RTMAX-3 RTMAX-2 RTMAX-1 RTMAX
    }
    set sigNum [expr $code - 128]
    if { $sigNum > 0 && $sigNum < [llength $signals] } {
        return [lindex $signals $sigNum]
    }
    return "unknown exit code \"$code\""
}

itcl::body Rappture::Task::get_uq {args} {
    foreach {path val} $args {
        if {$path == "-uq_type"} {
            set _uq(type) $val
        } elseif {$path == "-uq_args"} {
            set _uq(args) $val
        }
    }
    #set varlist [$_xmlobj uq_get_vars]
    foreach {varlist num} [$_xmlobj uq_get_vars] break
    return [Rappture::UQ ::#auto $varlist $num $_uq(type) $_uq(args)]
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
    # Make sure that we save the proper application name.  Actually, the
    # best place to get this information is straight from the "installtool"
    # script, but just in case we have an older tool, we should insert the
    # tool name from the resources config file.
    #
    if {[info exists _resources(-appname)] && $_resources(-appname) ne "" &&
        [$_xmlobj get tool.name] eq ""} {
        $_xmlobj put tool.name $_resources(-appname)
    }

    # if there are any args, use them to override parameters
    set _outputcb ""
    set _uq(type) ""
    foreach {path val} $args {
        if {$path == "-output"} {
            set _outputcb $val
        } elseif {$path == "-uq_type"} {
            set _uq(type) $val
        } elseif {$path == "-uq_args"} {
            set _uq(args) $val
        } else {
            $_xmlobj put $path.current $val
        }
    }

    # Initialize job array variables
    array set _job { 
        control  ""
        exitcode 0
        mesg     ""
        runfile  ""
        stderr   ""
        stdout   ""
        success  0
        xmlobj   ""
    }

    SetCpuResourceLimit 
    set driverFile [GetDriverFile]
    set cached 0
    if { [IsCacheable] } {
puts stderr "Cache checking: [time {
        set cached [CheckForCachedRunFile $driverFile] 
     } ]"
puts stderr "checking cache=$cached"
    }
    if { !$cached } {
        if { $_uq(type) != "" } {
            set _uq(tfile) [GetUQTemplateFile]
        }
        if { $_uq(type) == "" } {
            set cmd [GetSimulationCommand $driverFile]
        } else {
            set cmd [GetUQSimulationCommand $driverFile]
        }
        if { $cmd == "" } {
            puts stderr "cmd is empty"
            append mesg "There is no command specified by\n\n"
            append mesg "    <command>\n"
            append mesg "    </command>\n\n"
            append mesg "in the tool.xml file."
            return [list 1 $mesg]
        }
        Rappture::rusage mark
        if { ![ExecuteSimulationCommand $cmd] } {
            return [list 1 $_job(mesg)]
        }
        if { [resources -jobprotocol] == "submit" } {
            LogSubmittedSimulationUsage
        } else {
            LogSimulationUsage
        }
    } else {
        LogCachedSimulationUsage
    }
    if { $_job(success) } {
        file delete -force -- $driverFile
        Log run finished
        return [list 0 $_job(xmlobj)]
    } else {
        # See if the job was aborted.
        if {[regexp {^KILLED} $_job(control)]} {
            Log run aborted
            return [list 1 "ABORT"]
        }
        Log run failed [list 0 $_job(mesg)]
        return [list 1 $_job(mesg)]
    }
}

# ----------------------------------------------------------------------
#  Turn the command string from tool.xml into the proper syntax to use
#  with a submit parameter sweep with a temlate file.  Proper quoting
# of the template file is necessary to prevent submit from being too smart
# and converting it to a full pathname.
# ----------------------------------------------------------------------
itcl::body Rappture::Task::BuildSubmitCommand {cmd tfile params_file} {
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
# USAGE: abort
#
# Clients use this during a "run" to abort the current job.
# Kills the job and forces the "run" method to return.
# ----------------------------------------------------------------------
itcl::body Rappture::Task::abort {} {
    Log run abort
    set _job(control) "abort"
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

        # If there's a results_directory defined in the resources file,
        # then move the run.xml file there for storage.

        set rdir ""
        if {$resultdir eq "@default"} {
            if {[info exists _resources(-resultdir)]} {
                set rdir $_resources(-resultdir)
            } else {
                global rapptureInfo
                set rdir $rapptureInfo(cwd)
            }
        } elseif {$resultdir ne ""} {
            set rdir $resultdir
        }

        # use the runfile name generated by the last run
        if {$_job(runfile) ne ""} {
            set filename [file join $rdir $_job(runfile)]
        } else {
            set filename [file join $rdir run.xml]
        }
    }

    # add any last-minute metadata
    $xmlobj put output.time [clock format [clock seconds]]

    $xmlobj put tool.version.rappture.version $::Rappture::version
    $xmlobj put tool.version.rappture.revision $::Rappture::build
    $xmlobj put output.filename $filename
    $xmlobj put output.version $Rappture::version
    
    if {[info exists ::tcl_platform(user)]} {
        $xmlobj put output.user $::tcl_platform(user)
    }

    # save the output
    set rdir [file dirname $filename]
    file mkdir $rdir

    set fid [open $filename w]
    puts $fid "<?xml version=\"1.0\"?>"
    puts $fid [$xmlobj xml]
    close $fid

    Log output saved in $filename
}

# ----------------------------------------------------------------------
# USAGE: OnOutput <data>
#
# Used internally to send each bit of output <data> coming from the
# tool onto the caller, so the user can see progress.
# ----------------------------------------------------------------------
itcl::body Rappture::Task::OnOutput {data} {
    if {[string length $_outputcb] > 0} {
        uplevel #0 $_outputcb [list $data]
    }
}

# ----------------------------------------------------------------------
# USAGE: Log <cmd> <arg> <arg> ...
#
# Used internally to log interesting events during the run.  If the
# -logger option is set (to Rappture::Logger::log, or something like
# that), then the arguments to this method are passed along to the
# logger and written out to a log file.  Logging is off by default,
# so this method does nothing unless -logger is set.
# ----------------------------------------------------------------------
itcl::body Rappture::Task::Log {args} {
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

itcl::body Rappture::Task::IsCacheable {} {
    if { ![info exists _resources(-cachehosts)] ||
         $_resources(-cachehosts) == "" } {
        puts stderr cachehosts=[info exists _resources(-cachehosts)]
        return 0
    }
    global env
    if { [info exists env(RAPPTURE_CACHE_OVERRIDE)] } {
        set state $env(RAPPTURE_CACHE_OVERRIDE)
    } else {
        set state [$_xmlobj get "tool.cache"]
    }
    puts stderr "cache tag is \"$state\""
    if { $state == ""  || ![string is boolean $state] } {
        return 1;                       # Default is to allow caching.
    }
    return $state
}

#
# Send the list of parameters to a python program so it can call PUQ
# and get a CSV file containing the parameter values to use for the runs.
itcl::body Rappture::Task::GetParamsForUQ {} {
    set pid [pid]
    # puts "puq.sh get_params $pid $_uq(varlist) $_uq(type) $_uq(args)"
    if {[catch {
        exec puq.sh get_params $pid $_uq(varlist) $_uq(type) $_uq(args)
    } errs] != 0 } {
        error "get_params.py failed: $errs\n[GetUQErrors]"
    }
    return "params${pid}.csv"
}

itcl::body Rappture::Task::SetCpuResourceLimit {} {
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
}

# Write out the driver.xml file for the tool
itcl::body Rappture::Task::GetDriverFile {} {
    global rapptureInfo
    set fileName [file join $rapptureInfo(cwd) "driver[pid].xml"]
    if { [catch {
        set f [open $fileName w]
        puts $f "<?xml version=\"1.0\"?>"
        puts $f [$_xmlobj xml]
        close $f
    } errs] != 0 } {
        error "can't create driver file \"$fileName\": $errs"
    }
    return $fileName
}

itcl::body Rappture::Task::GetCommand { } {
    set cmd [$_xmlobj get tool.command]
    regsub -all @tool $cmd $_installdir cmd
    set cmd [string trimleft $cmd " "]
    return $cmd
}

itcl::body Rappture::Task::GetSimulationCommand { driverFile } {
    set cmd [GetCommand]
    if { $cmd == "" } {
        return ""
    }
    regsub -all @driver $cmd $driverFile cmd

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
    return $cmd
}

itcl::body Rappture::Task::GetUQSimulationCommand { driverFile } {
    set cmd [GetCommand]
    if { $cmd == "" } {
        return ""
    }
    set paramsFile [GetParamsForUQ]
    set cmd [BuildSubmitCommand $cmd $_uq(tfile) $paramsFile]
    file delete -force puq
    return $cmd
}

itcl::body Rappture::Task::GetUQTemplateFile {} {
    global rapptureInfo
    # Copy xml into a new file
    set templateFile "template[pid].xml"
    set f [open $templateFile w]
    puts $f "<?xml version=\"1.0\"?>"
    puts $f [$_xmlobj xml]
    close $f

    # Return a list of the UQ variables and their PDFs.
    # Also turns $uq(tfile) into a template file.
    set _uq(varlist) [lindex [$_xmlobj uq_get_vars $templateFile] 0]
    set _uq(tfile) $templateFile
    return $templateFile
}

itcl::body Rappture::Task::ExecuteSimulationCommand { cmd } {

    set _job(runfile) ""
    set _job(success) 0
    set _job(exitcode) 0

    # Step 1.  Write the command into the run file.
    $_xmlobj put tool.execute $cmd

    Log run started
    Rappture::rusage mark

    # Step 2.  Check if it is a special case "ECHO" command which always
    #          succeeds.
    if { [string compare -nocase -length 5 $cmd "ECHO "] == 0 } {
        set _job(stdout) [string range $cmd 5 end]
        set _job(success) 1
        set _job(exitcode) 0
        set _job(mesg) ""
        return 1;                       # Success
    } 

    # Step 3. Execute the command, collecting its stdout and stderr.
    catch {
        eval blt::bgexec [list [itcl::scope _job(control)]] \
            -keepnewline yes \
            -killsignal  SIGTERM \
            -onoutput    [list [itcl::code $this OnOutput]] \
            -output      [list [itcl::scope _job(stdout)]] \
            -error       [list [itcl::scope _job(stderr)]] \
            $cmd
    } result

    # Step 4. Check the token and the exit code.
    set logmesg $result
    foreach { token _job(pid) _job(exitcode) mesg } $_job(control) break
    if { $token == "EXITED" } {
        if { $_job(exitcode) != 0 } {
            # This means that the program exited normally but returned a
            # non-zero exitcode.  Consider this an invalid result from the
            # program.  Append the stderr from the program to the message.
            if {$_job(exitcode) > 128} {
                set logmesg "Program signaled: signal was [GetSignal $_job(exitcode)]"
            } else { 
                set logmesg "Program finished: non-zero exit code is $_job(exitcode)"
            }
            set _job(mesg) "$logmesg\n\n$_job(stderr)"
            Log run failed [list $logmesg]
            return 0;                   # Fail.
        }
        # Successful program termination with exit code of 0.
    } elseif { $token == "abort" }  {
        # The user pressed the abort button.
        
        set logmesg "Program terminated by user."
        Log run failed [list $logmesg]
        set _job(mesg) "$logmesg\n\n$_job(stdout)"
        return 0;                       # Fail
    } else {
        # Abnormal termination
        
        set logmesg "Abnormal program termination:"
        Log run failed [list $logmesg]
        set _job(mesg) "$logmesg\n\n$_job(stdout)"
        return 0;                       # Fail
    }
    if { $_uq(type) != "" } {
        CollectUQResults
    }

    # Step 5. Look in stdout for the name of the run file.
    set pattern {=RAPPTURE-RUN=>([^\n]+)}
    if {![regexp $pattern $_job(stdout) match fileName]} {
        set _job(mesg) "Can't find result file in output.\n"
        append _job(mesg) "Did you call Rappture::result in your simulator?"
        return 0;                       # Fail
    }
    set _job(runfile) $fileName
    set _job(success) 1
    set _job(mesg) $_job(stdout)
    return 1;                           # Success
}

itcl::body Rappture::Task::LogSimulationUsage {} {
    array set times [Rappture::rusage measure]

    set toolId     [$_xmlobj get tool.id]
    set toolVers   [$_xmlobj get tool.version.application.revision]
    set simulation "simulation"
    if { $toolId ne "" && $toolVers ne "" } {
        set simulation "[pid]_${toolId}_r${toolVers}"
    }
    
    # Need to save job info? then invoke the callback
    if { [string length $jobstats] > 0} {
        lappend args \
            "job"      [incr jobnum] \
            "event"    $simulation \
            "start"    $times(start) \
            "walltime" $times(walltime) \
            "cputime"  $times(cputime) \
            "status"   $_job(exitcode)
        uplevel #0 $jobstats $args 
    }
        
    #
    # Scan through stderr channel and look for statements that
    # represent grid jobs that were executed.  The statements look
    # like this:
    #
    # MiddlewareTime: job=1 event=simulation start=3.001094 ...
    #
    
    set subjobs 0
    set pattern {(^|\n)MiddlewareTime:( +[a-z]+=[^ \n]+)+(\n|$)}
    while { [regexp -indices $pattern $_job(stderr) match] } {
        foreach {p0 p1} $match break
        if { [string index $_job(stderr) $p0] == "\n" } {
            incr p0
        }
        array unset data
        array set data {
            job 1
            event simulation
            start 0
            walltime 0
            cputime 0
            status 0
        }
        foreach arg [lrange [string range $_job(stderr) $p0 $p1] 1 end] {
            foreach {key val} [split $arg =] break
            set data($key) $val
        }
        set data(job)   [expr { $jobnum + $data(job) }]
        set data(event) "subsimulation"
        set data(start) [expr { $times(start) + $data(start) }]
        
        set details ""
        foreach key {job event start walltime cputime status} {
            # Add required keys in a particular order
            lappend details $key $data($key)
            unset data($key)
        }
        foreach key [array names data] {
            # Add anything else that the client gave -- venue, etc.
            lappend details $key $data($key)
        }
        
        if {[string length $jobstats] > 0} {
            uplevel #0 $jobstats $details
        }
        
        incr subjobs
        
        # Done -- remove this statement
        set _job(stderr) [string replace $_job(stderr) $p0 $p1]
    }
    incr jobnum $subjobs

    # Add cputime info to run.xml file
    if { [catch {
        Rappture::library $_job(runfile)
    } xmlobj] != 0 } {
        error "Can't create rappture library: $xmlobj"
    }
    $xmlobj put output.walltime $times(walltime)
    $xmlobj put output.cputime $times(cputime)
    global env
    if {[info exists env(SESSION)]} {
        $xmlobj put output.session $env(SESSION)
    }
    set _job(xmlobj) $xmlobj
}

itcl::body Rappture::Task::LogSubmittedSimulationUsage {} {
    array set times [Rappture::rusage measure]

    set toolId     [$_xmlobj get tool.id]
    set toolVers   [$_xmlobj get tool.version.application.revision]
    set simulation "simulation"
    if { $toolId ne "" && $toolVers ne "" } {
        set simulation "[pid]_${toolId}_r${toolVers}"
    }
    
    # Need to save job info? then invoke the callback
    if { [string length $jobstats] > 0} {
        lappend args \
            "job"      [incr jobnum] \
            "event"    $simulation \
            "start"    $times(start) \
            "walltime" $times(walltime) \
            "cputime"  $times(cputime) \
            "status"   $_job(exitcode)
        uplevel #0 $jobstats $args 
    }
        
    #
    # Scan through stderr channel and look for statements that
    # represent grid jobs that were executed.  The statements look
    # like this:
    #
    # MiddlewareTime: job=1 event=simulation start=3.001094 ...
    #
    
    set subjobs 0
    set pattern {(^|\n)MiddlewareTime:( +[a-z]+=[^ \n]+)+(\n|$)}
    while { [regexp -indices $pattern $_job(stderr) match] } {
        foreach {p0 p1} $match break
        if { [string index $_job(stderr) $p0] == "\n" } {
            incr p0
        }
        array unset data
        array set data {
            job 1
            event simulation
            start 0
            walltime 0
            cputime 0
            status 0
        }
        foreach arg [lrange [string range $_job(stderr) $p0 $p1] 1 end] {
            foreach {key val} [split $arg =] break
            set data($key) $val
        }
        set data(job)   [expr { $jobnum + $data(job) }]
        set data(event) "subsimulation"
        set data(start) [expr { $times(start) + $data(start) }]
        
        set details ""
        foreach key {job event start walltime cputime status} {
            # Add required keys in a particular order
            lappend details $key $data($key)
            unset data($key)
        }
        foreach key [array names data] {
            # Add anything else that the client gave -- venue, etc.
            lappend details $key $data($key)
        }
        
        if {[string length $jobstats] > 0} {
            uplevel #0 $jobstats $details
        }
        
        incr subjobs
        
        # Done -- remove this statement
        set _job(stderr) [string replace $_job(stderr) $p0 $p1]
    }
    incr jobnum $subjobs

    # Add cputime info to run.xml file
    if { [catch {
        Rappture::library $_job(runfile)
    } xmlobj] != 0 } {
        error "Can't create rappture library: $xmlobj"
    }
    global env
    if {[info exists env(SESSION)]} {
        $xmlobj put output.session $env(SESSION)
    }
    set _job(xmlobj) $xmlobj
}

itcl::body Rappture::Task::LogCachedSimulationUsage {} {
    if { [catch {
        Rappture::library $_job(runfile)
    } xmlobj] != 0 } {
        error "Can't create rappture library: $xmlobj"
    }
    # Get the session from runfile 
    set session [$xmlobj get "output.session"]
    if { [catch {exec submit --cache $session} result] != 0 } {
       puts stderr "submit --cache failed: $result"
    }
    set _job(xmlobj) $xmlobj
}


itcl::body Rappture::Task::CheckForCachedRunFile { driverFile } {

    # Read the driver file and collect its contents as the query.
    set url http://$_resources(-cachehosts)/cache/request
    set f [open $driverFile "r"]
    set query [read $f]
    close $f

    # Make the query
    if { [catch {
        http::geturl $url -query $query -timeout 6000 -binary yes
    } token] != 0 } {
        puts stderr "error performing cache query: token=$token"
        return 0
    }
    # If the code isn't 200, we'll assume it's a cache miss.
    if { [http::ncode $token] != 200} {
        return 0
    }
    # Get contents of the run file. 
    set contents [http::data $token]
    if { $contents == "" } {
        return 0
    }

    # Create a new run.xml file and write the results into it. 
    set secs [clock seconds]
    set millisecs [expr [clock clicks -milliseconds] % 1000]
    set timestamp [format %d%03d%03d $secs $millisecs 0]

    global rapptureInfo
    set fileName [file join $rapptureInfo(cwd) "run${timestamp}.xml"]
    set f [open $fileName "w"]
    puts $f $contents
    close $f
    set _job(runfile) $fileName
    set _job(success) 1
    set _job(stderr) "Loading cached results\n"
    OnOutput "Loading cached results\n"
    update
    return 1
}

itcl::body Rappture::Task::GetUQErrors {} {
    set contents {}
    if { [file exists "uq_debug.err"] } {
        set f [open "uq_debug.err" r]
        set contents [read $f]
        close $f
    }
    return $contents
}

# UQ. Collect data from all jobs and put it in one xml run file.
itcl::body Rappture::Task::CollectUQResults {} {
    file delete -force -- "run_uq.xml"
    set hdfFile puq_[pid].hdf5
    if { [catch {
        exec puq.sh analyze $hdfFile
    } results] != 0 } {
        error "UQ analysis failed: $results\n[GetUQErrors]"
    } else {
        set _job(stdout) $results
    }
}
