# ----------------------------------------------------------------------
#  COMPONENT: test - run a test and query the results
#
#  Encapsulates the testing logic, to keep it isolated from the rest of
#  the tester GUI.  Constructor requires the location of the tool.xml
#  for the new version, and the test xml file containing the golden set
#  of results. 
# ======================================================================
#  AUTHOR:  Ben Rafferty, Purdue University
#  Copyright (c) 2010  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================

namespace eval Rappture::Tester { #forward declaration }

itcl::class Rappture::Tester::Test {
    public variable notifycommand ""

    constructor {tool testxml args} { #defined later }
    destructor { #defined later }

    public method getResult {}
    public method getTestInfo {path}
    public method getDiffs {}

    public method getRunobj {}
    public method getTestobj {}

    public method run {args}
    public method regoldenize {}

    private variable _toolobj ""  ;# Rappture::Tool for tool being tested
    private variable _testxml ""  ;# XML file for test case
    private variable _testobj ""  ;# Rappture::Library object for _testxml

    private variable _result "?"  ;# current status of this test
    private variable _runobj ""   ;# results from last run
    private variable _diffs ""    ;# diffs with respect to _runobj

    private method _setWaiting {{newval ""}}
    private method _setResult {name}
    private method _computeDiffs {obj1 obj2 args}
    private method _getStructure {xmlobj path}

    # use this to add tests to the "run" queue
    public proc queue {op args}

    private common _queue       ;# queue of objects waiting to run
    set _queue(tests) ""        ;# list of tests in the queue
    set _queue(pending) ""      ;# after event for "next" call
    set _queue(running) ""      ;# test object currently running
    set _queue(outputcmd) ""    ;# callback for reporting output
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Test::constructor {toolobj testxml args} {
    set _toolobj $toolobj

    set _testxml $testxml
    set _testobj [Rappture::library $testxml]

    # HACK: Add a new input to differentiate between results
    $_testobj put input.TestRun.current "Golden"

    eval configure $args
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Test::destructor {} {
    # TODO: toolobj is created in main.tcl and passed into all tests
    #  when they are created.  Should we really delete it here whenever
    #  a test is destroyed?
    itcl::delete object $_toolobj
    itcl::delete object $_testobj
    if {$_runobj ne ""} {
        itcl::delete object $_runobj
    }
}

# ----------------------------------------------------------------------
# USAGE: getResult
#
# Returns the result of the test:
#   ? ...... test hasn't been run yet
#   Pass ... test ran recently and passed
#   Fail ... test ran recently and failed
#   Error ... test ran recently and run failed with an error
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Test::getResult {} {
    return $_result
}

# ----------------------------------------------------------------------
# USAGE: getTestInfo <path>
# 
# Returns info about the Test case at the specified <path> in the XML.
# If the <path> is missing or misspelled, this method returns ""
# instead of an error.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Test::getTestInfo {path} {
    return [$_testobj get $path]
}

# ----------------------------------------------------------------------
# USAGE: run ?-output callback path value path value ...?
#
# Kicks off a new simulation and checks the results against the golden
# set of results.  Any arguments passed in are passed along to the
# Tool object managing the run.  This may include parameter override
# values and a callback for partial output.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Test::run {args} {
    # Delete existing library if rerun
    if {$_runobj ne ""} {
        itcl::delete object $_runobj
        set _runobj ""
        set _diffs ""
    }

    # copy inputs from the test into the run file
    $_toolobj reset
    foreach path [Rappture::entities -as path $_testobj input] {
        if {[$_testobj element -as type $path.current] ne ""} {
puts "  override: $path = [$_testobj get $path.current]"
            lappend args $path [$_testobj get $path.current]
        }
    }

    # run the test case...
    _setResult "Running"
    foreach {status result} [eval $_toolobj run $args] break

    if {$status == 0} {
        if {$result eq "ABORT"} {
            _setResult "?"
            return "aborted"
        } elseif {[Rappture::library isvalid $result]} {
            set _runobj $result
            set _diffs [_computeDiffs $_testobj $_runobj -section output]
puts "DIFFS:"
foreach {op path what v1 v2} $_diffs {
puts "  $op $path ($what) $v1 $v2"
}

            # HACK: Add a new input to differentiate between results
            $_runobj put input.TestRun.current "Test result"

            # any differences from expected result mean test failed
            if {[llength $_diffs] == 0} {
                _setResult "Pass"
            } else {
                _setResult "Fail"
            }
            return "finished"
        } else {
            _setResult "Fail"
            return "failed: $result"
        }
    } else {
        _setResult "Fail"
        tk_messageBox -icon error -message "Tool failed: $result"
        return "finished"
    }
}

# ----------------------------------------------------------------------
# USAGE: regoldenize
#
# Regoldenize the test by overwriting the test xml containin the golden
# results with the data in the runfile generated by the last run.  Copy
# test label and description into the new file.  Update the test's
# result attributes to reflect the changes. Throws an error if the test
# has not been run.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Test::regoldenize {} {
    if {$_runobj eq ""} {
        error "no test result to goldenize"
    }
    $_runobj put test.label [$_testobj get test.label]
    $_runobj put test.description [$_testobj get test.description]

    set fid [open $_testxml w]
    puts $fid "<?xml version=\"1.0\"?>"
    puts $fid [$_runobj xml]
    close $fid

    itcl::delete object $_testobj
    set _testobj $_runobj

    set _runobj ""
    set _diffs ""
    _setResult Pass
}

# ----------------------------------------------------------------------
# USAGE: getDiffs
#
# Returns a list of paths that exist in both the golden and new results,
# but contain data that does not match according to the compareElements
# method.  Throws an error if the test has not been run.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Test::getDiffs {} {
    return $_diffs
}

# -----------------------------------------------------------------------
# USAGE: getRunobj
#
# Returns the library object generated by the previous run of the test.
# Throws an error if the test has not been run.
# -----------------------------------------------------------------------
itcl::body Rappture::Tester::Test::getRunobj {} {
    if {$_runobj eq ""} {
        error "Test has not yet been run."
    }
    return $_runobj
}

# ----------------------------------------------------------------------
# USAGE: getTestobj
#
# Returns a library object representing the test case.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Test::getTestobj {} {
    return $_testobj
}

# ----------------------------------------------------------------------
# USAGE: _setResult ?|Pass|Fail|Waiting|Running
#
# Used internally to change the state of this test case.  If there
# is a -notifycommand script for this object, it is invoked to notify
# an interested client that the object has changed.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Test::_setResult {name} {
    set _result $name
    if {[string length $notifycommand] > 0} {
        uplevel #0 $notifycommand $this
    }
}

# ----------------------------------------------------------------------
# USAGE: _setWaiting ?boolean?
#
# Used to mark a Test as "waiting".  This usually happens when a test
# is added to the queue, about to be run.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Test::_setWaiting {{newval ""}} {
    if {$newval ne "" && [string is boolean $newval]} {
        if {$newval} {
            _setResult "Waiting"
        } else {
            _setResult "?"
        }
    }
    return $_result
}

# ----------------------------------------------------------------------
# USAGE: _computeDiffs <xmlObj1> <xmlObj2> ?-section xxx? \
#            ?-what value|structure|all?
#
# Used internally to compute differences between two different XML
# objects.  This is normally used to look for differences in the
# output section between a test case and a new run, but can also
# be used to look for differences in other sections via the -section
# flag.
#
# Returns a list of the following form:
#     <op> <path> <what> <val1> <val2>
#
#       where <op> is one of:
#         - ...... element is missing from <xmlObj2>
#         c ...... element changed between <xmlObj1> and <xmlObj2>
#         + ...... element is missing from <xmlObj1>
#
#       and <what> is something like:
#         value .............. difference affects "current" value 
#         structure <path> ... affects structure of parent at <path>
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Test::_computeDiffs {obj1 obj2 args} {
    Rappture::getopts args params {
        value -section output
        value -what all
    }
    if {$params(-what) == "all"} {
        set params(-what) "structure value"
    }

    # query the values for all entities in both objects
    set v1paths [Rappture::entities $obj1 $params(-section)]
    set v2paths [Rappture::entities $obj2 $params(-section)]

    # scan through values for obj1 and compare against obj2
    set rlist ""
    foreach path $v1paths {
puts "checking $path"
        set i [lsearch -exact $v2paths $path]
        if {$i < 0} {
puts "  missing from $obj2"
            # missing from obj2
            foreach {raw norm} [Rappture::LibraryObj::value $obj1 $path] break
            lappend rlist - $path value $raw ""
        } else {
            foreach part $params(-what) {
                switch -- $part {
                  value {
                    foreach {raw1 norm1} \
                        [Rappture::LibraryObj::value $obj1 $path] break
                    foreach {raw2 norm2} \
                        [Rappture::LibraryObj::value $obj2 $path] break
puts "  checking values $norm1 vs $norm2"
                    if {![string equal $norm1 $norm2]} {
puts "  => different!"
                        # different from obj2
                        lappend rlist c $path value $raw1 $raw2
                    }
                    # handled this comparison
                    set v2paths [lreplace $v2paths $i $i]
                  }
                  structure {
                    set what [list structure $path]
                    set s1paths [_getStructure $obj1 $path]
                    set s2paths [_getStructure $obj2 $path]
                    foreach spath $s1paths {
puts "  checking internal structure $spath"
                        set i [lsearch -exact $s2paths $spath]
                        if {$i < 0} {
puts "    missing from $obj2"
                            # missing from obj2
                            set val1 [$obj1 get $spath]
                            lappend rlist - $spath $what $val1 ""
                        } else {
                            set val1 [$obj1 get $spath]
                            set val2 [$obj2 get $spath]
                            if {![string match $val1 $val2]} {
puts "    different from $obj2 ($val1 vs $val2)"
                                # different from obj2
                                lappend rlist c $spath $what $val1 $val2
                            }
                            # handled this comparison
                            set s2paths [lreplace $s2paths $i $i]
                        }
                    }

                    # look for leftover values
                    foreach spath $s2paths {
                        set val2 [$obj2 get $spath]
puts "    extra $spath in $obj2"
                        lappend rlist + $spath $what "" $val2
                    }
                  }
                  default {
                    error "bad part \"$part\": should be structure, value"
                  }
                }
            }
        }
    }

    # add any values left over in the obj2
    foreach path $v2paths {
puts "    extra $path in $obj2"
        foreach {raw2 norm2} [Rappture::LibraryObj::value $obj2 $path] break
        lappend rlist + $path value "" $raw2
    }
    return $rlist
}

# ----------------------------------------------------------------------
# USAGE: _getStructure <xmlObj> <path>
#
# Used internally by _computeDiffs to get a list of paths for important
# parts of the internal structure of an object.  Avoids the "current"
# element, but includes "default", "units", etc.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Test::_getStructure {xmlobj path} {
    set rlist ""
    set queue $path
    while {[llength $queue] > 0} {
        set qpath [lindex $queue 0]
        set queue [lrange $queue 1 end]

        foreach p [$xmlobj children -as path $qpath] {
            if {[string match *.current $p]} {
                continue
            }
            if {[llength [$xmlobj children $p]] > 0} {
                # continue exploring nodes with children
                lappend queue $p
            } else {
                # return the terminal nodes
                lappend rlist $p
            }
        }
    }
    return $rlist
}

# ======================================================================
# RUN QUEUE
# ======================================================================
# USAGE: queue add <testObj> <testObj>...
# USAGE: queue clear ?<testObj> <testObj>...?
# USAGE: queue status <command>
# USAGE: queue next
# USAGE: queue output <string>
#
# Used to manipulate the run queue for the program as a whole.
#
# The "queue add" option adds the given <testObj> objects to the run
# queue.  As soon as an object is added to the queue, it is marked
# "waiting".  When it runs, it is marked "running", and it finally
# goes to the "pass" or "fail" state.  If an object is already in
# the queue, then this operation does nothing.
#
# The "queue clear" option clears specific objects from the queue.
# If no objects are specified, then it clears all remaining objects.
#
# The "queue status" option is used to set the callback for handling
# output from runs.  This command is called two ways:
#    command start <testObj>
#    command add <testObj> "string of output"
#
# The "queue next" option is used internally to run the next object
# in the queue.  The "queue output" option is also used internally
# to handle the output coming back from a run.  The output gets
# shuttled along to the callback specified by "queue status".
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Test::queue {option args} {
    switch -- $option {
        add {
            # add these tests to the run queue
            foreach obj $args {
                if {[catch {$obj isa Rappture::Tester::Test} valid] || !$valid} {
                    error "bad value \"$obj\": should be Test object"
                }
                if {[lsearch $_queue(tests) $obj] < 0} {
                    $obj _setWaiting 1
                    lappend _queue(tests) $obj
                }
            }
            if {$_queue(running) eq "" && $_queue(pending) eq ""} {
                set _queue(pending) [after idle \
                    Rappture::Tester::Test::queue next]
            }
        }
        clear {
            # remove these tests from the run queue
            foreach obj $args {
                if {[catch {$obj isa Rappture::Tester::Test} valid] || !$valid} {
                    error "bad value \"$obj\": should be Test object"
                }

                # remove the test from the queue
                set i [lsearch $_queue(tests) $obj]
                if {$i >= 0} {
                    set _queue(tests) [lreplace $_queue(tests) $i $i]
                }

                # mark object as no longer "waiting"
                if {[$obj _setWaiting]} {
                    $obj _setWaiting 0
                }
            }
        }
        status {
            if {[llength $args] != 1} {
                error "wrong # args: should be \"status command\""
            }
            set _queue(outputcmd) [lindex $args 0]
        }
        next {
            set _queue(pending) ""

            # get the next object from the queue
            set obj [lindex $_queue(tests) 0]
            set _queue(tests) [lrange $_queue(tests) 1 end]

            if {$obj ne ""} {
                set _queue(running) $obj
                # invoke the callback to signal start of a run
                if {[string length $_queue(outputcmd)] > 0} {
                    uplevel #0 $_queue(outputcmd) start $obj
                }

                # run the test
                set callback "Rappture::Tester::Test::queue output"
                set status [$obj run -output $callback]
                set _queue(running) ""

                if {$status == "aborted"} {
                    # if the test was aborted, clear any waiting tests
                    Rappture::Tester::Test::queue clear
                } elseif {[string match failed:* $status]} {
                    bgerror $status
                }

                # set up to run the next test in the queue
                set _queue(pending) [after idle \
                    Rappture::Tester::Test::queue next]
            }
        }
        output {
            if {[llength $args] != 1} {
                error "wrong # args: should be \"output string\""
            }
            if {[string length $_queue(outputcmd)] > 0} {
                uplevel #0 $_queue(outputcmd) add $_queue(running) $args
            }
        }
        default {
            error "bad option \"$option\": should be add, clear, status, output, next"
        }
    }
}
