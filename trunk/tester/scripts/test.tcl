# ----------------------------------------------------------------------
#  COMPONENT: test - run a test and query the results
#
#  Encapsulates the testing logic, to keep it isolated from the rest of
#  the tester GUI.  Constructor requires the location of the tool.xml
#  for the new version, and the test xml file containing the golden set
#  of results. 
# ======================================================================
#  AUTHOR:  Ben Rafferty, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
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
    public method getTestInfo {args}
    public method getRunInfo {args}
    public method getDiffs {args}

    public method getTestobj {}
    public method getTestxml {}

    public method run {args}
    public method abort {}
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
    private method _buildFailure {str}

    # use this to add tests to the "run" queue
    public proc queue {op args}

    # useful helper function -- looks for val among choices
    public proc oneof {choices val} {
        return [expr {[lsearch -exact $choices $val] >= 0}]
    }

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

    eval configure $args
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Test::destructor {} {
    itcl::delete object $_testobj
    # runobj can point to testobj if the test has just been
    # regoldenized.  Don't try to delete twice.
    if {$_runobj ne "" && $_runobj ne $_testobj} {
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
# USAGE: getTestInfo children <path>
# USAGE: getTestInfo element ?-as type? <path>
# 
# Returns info about the Test case at the specified <path> in the XML.
# If the <path> is missing or misspelled, this method returns ""
# instead of an error.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Test::getTestInfo {args} {
    if {[llength $args] == 1} {
        set path [lindex $args 0]
        return [$_testobj get $path]
    }
    return [eval $_testobj $args]
}

# ----------------------------------------------------------------------
# USAGE: getRunInfo <path>
# USAGE: getRunInfo children <path>
# USAGE: getRunInfo element ?-as type? <path>
# 
# Returns info about the most recent run at the specified <path> in
# the XML.  If the <path> is missing or misspelled, this method returns
# "" instead of an error.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Test::getRunInfo {args} {
    if {[llength $args] == 1} {
        set path [lindex $args 0]
        return [$_runobj get $path]
    }
    return [eval $_runobj $args]
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

            if {![oneof {0 ok} [$_testobj get output.status]]} {
                # expected test to fail, but it didn't
                set idiffs [_computeDiffs [$_toolobj xml object] $_runobj -in input]
                set odiffs [_computeDiffs $_testobj $_runobj -what run]
                set _diffs [concat $idiffs $odiffs]
                _setResult "Fail"
            } else {
                set idiffs [_computeDiffs [$_toolobj xml object] $_testobj -in input]
                set odiffs [_computeDiffs $_testobj $_runobj -in output]
                set _diffs [concat $idiffs $odiffs]

                # any differences from expected result mean test failed
                if {[llength $_diffs] == 0} {
                    _setResult "Pass"
                } else {
                    _setResult "Fail"
                }
            }
            return "finished"
        } else {
            set _runobj [_buildFailure $result]
            if {![oneof {0 ok} [$_testobj get output.status]]
                  && [$_testobj get output.log] eq $result} {
                _setResult "Pass"
            } else {
                set idiffs [_computeDiffs [$_toolobj xml object] $_runobj -in input]
                set odiffs [_computeDiffs $_testobj $_runobj -what run]
                set _diffs [concat $idiffs $odiffs]
                _setResult "Fail"
            }
            return "finished"
        }
    } else {
        set _runobj [_buildFailure $result]
        if {![oneof {0 ok} [$_testobj get output.status]]
              && [$_testobj get output.log] eq $result} {
            _setResult "Pass"
        } else {
            set idiffs [_computeDiffs [$_toolobj xml object] $_runobj -in input]
            set odiffs [_computeDiffs $_testobj $_runobj -what run]
            set _diffs [concat $idiffs $odiffs]
            _setResult "Fail"
        }
        return "finished"
    }
}

# ----------------------------------------------------------------------
# USAGE: abort
#
# Causes the current test kicked off by the "run" method to be aborted.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Test::abort {} {
    $_toolobj abort
}

# ----------------------------------------------------------------------
# USAGE: regoldenize
#
# Regoldenize the test by overwriting the test xml containin the golden
# results with the data in the runfile generated by the last run.  Copy
# test label and description into the new file.  Update the test's
# result attributes to reflect the changes. Throws an error if the test
# has not been run.
#
# After regoldenizing, _testobj and _runobj will both refer to the same
# library object, and the previous _runobj will be discarded.
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
# USAGE: getDiffs ?<path>?
#
# With no extra args, this returns a list of paths that differ between
# the golden and new results--either because the data values are
# different, or because elements are missing or their attributes have
# changed.
#
# If a particular <path> is specified, then detailed diffs are returned
# for that path.  This is useful for "structure" diffs, where many
# things may be different within a single object.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Test::getDiffs {args} {
    if {[llength $args] == 0} {
        return $_diffs
    } elseif {[llength $args] != 1} {
        error "wrong # args: should be \"getDiffs ?path?\""
    }

    set path [lindex $args 0]
    if {[string match input.* $path]} {
        # if we're matching input, compare the original XML vs. the test
        return [_computeDiffs [$_toolobj xml object] $_testobj -in $path -detail max]
    }

    # otherwise, compare the golden test vs. the test result
    return [_computeDiffs $_testobj $_runobj -in $path -detail max]
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
# USAGE: getTestxml
#
# Returns the name of the xml file representing the test case.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Test::getTestxml {} {
    return $_testxml
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
# USAGE: _computeDiffs <xmlObj1> <xmlObj2> ?-in xxx? \
#            ?-what value|attrs|run|all? ?-detail min|max?
#
# Used internally to compute differences between two different XML
# objects.  This is normally used to look for differences between an
# entire test case and a new run, but can also be used to look at
# differences within a particular section or element via the -in flag.
#
# Returns a list of the following form:
#     -what <diff> -path <path> -obj1 <xmlobj> -obj2 <xmlobj> \
#           -v1 <value1> -v2 <value2>
#
#       where <diff> is one of:
#         value - ....... element is missing from <xmlObj2>
#         value c ....... element changed between <xmlObj1> and <xmlObj2>
#         value + ....... element is missing from <xmlObj1>
#         attrs c ....... attributes are different <xmlObj1> and <xmlObj2>
#         type c ........ object types are different <xmlObj1> and <xmlObj2>
#         attr - <path>.. attribute at <path> is missing from <xmlObj2>
#         attr + <path>.. attribute at <path> is missing from <xmlObj1>
#         attr c <path>.. attribute at <path> changed between objects
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Test::_computeDiffs {obj1 obj2 args} {
    Rappture::getopts args params {
        value -in output
        value -what "attrs value"
        value -detail min
    }
    if {$params(-what) == "all"} {
        set params(-what) "attrs value run"
    }

    # handle any run output diffs first, so they appear at the top
    # report this as one incident -- not separate reports for status/log
    set rlist ""
    if {[lsearch $params(-what) "run"] >= 0} {
        set st1 [$obj1 get output.status]
        set st2 [$obj2 get output.status]
        if {$st1 ne $st2} {
            # status changes are most serious
            lappend rlist [list -what status -path output.status \
                -obj1 $obj1 -obj2 $obj2]
        } else {
            set log1 [$obj1 get output.log]
            set log2 [$obj2 get output.log]
            if {$log1 ne $log2} {
                # flag log changes instead if status agrees
                lappend rlist [list -what status -path output.log \
                    -obj1 $obj1 -obj2 $obj2]
            }
        }
    }

    # scan through the specified sections or paths
    foreach elem $params(-in) {
        if {[string first . $elem] >= 0} {
            set v1paths $elem
            set v2paths $elem
        } else {
            # query the values for all entities in both objects
            set v1paths [Rappture::entities $obj1 $elem]
            set v2paths [Rappture::entities $obj2 $elem]
        }

        # scan through values for obj1 and compare against obj2
        foreach path $v1paths {
            set details [list -path $path -obj1 $obj1 -obj2 $obj2]

            set i [lsearch -exact $v2paths $path]
            if {$i < 0} {
                # missing from obj2
                lappend rlist [linsert $details 0 -what "value -"]
            } else {
                foreach part $params(-what) {
                    switch -- $part {
                      value {
                        set val1 [Rappture::objects::import $obj1 $path]
                        set val2 [Rappture::objects::import $obj2 $path]
                        lappend details -val1 $val1 -val2 $val2

                        if {$val1 eq "" || $val2 eq ""} {
                            lappend rlist [linsert $details 0 -what "value c"]
                        } elseif {[$val1 info class] != [$val2 info class]} {
                            lappend rlist [linsert $details 0 -what "value c"]
                        } elseif {[$val1 compare $val2] != 0} {
                            lappend rlist [linsert $details 0 -what "value c"]
                        } else {
                            itcl::delete object $val1 $val2
                        }
                        # handled this comparison
                        set v2paths [lreplace $v2paths $i $i]
                      }
                      attrs {
                        set what [list structure $path]
                        set type1 [$obj1 element -as type $path]
                        set type2 [$obj2 element -as type $path]
                        if {$type1 eq $type2} {
                          set same yes
                          if {[catch {Rappture::objects::get $type1 -attributes} alist]} {
                              # oops! unknown object type
                              lappend rlist [linsert $details 0 -what unkobj]
                              set alist ""
                          }
                          foreach rec $alist {
                              array set attr [lrange $rec 1 end]
                              set apath $path.$attr(-path)
                              set v1 [$obj1 get $apath]
                              set v2 [$obj2 get $apath]
                              set dt [linsert $details end -v1 $v1 -v2 $v2]

                              if {$v2 eq "" && $v1 ne ""} {
                                  # missing from obj2
                                  if {$params(-detail) == "max"} {
                                      lappend rlist [linsert $dt 0 -what [list attr - $attr(-path)]]
                                  } else {
                                      set same no
                                      break
                                  }
                              } elseif {$v1 eq "" && $v2 ne ""} {
                                  # missing from obj1
                                  if {$params(-detail) == "max"} {
                                      lappend rlist [linsert $dt 0 -what [list attr + $attr(-path)]]
                                  } else {
                                      set same no
                                      break
                                  }
                              } elseif {$v1 ne $v2} {
                                  # different from obj2
                                  if {$params(-detail) == "max"} {
                                      lappend rlist [linsert $dt 0 -what [list attr c $attr(-path)]]
                                  } else {
                                      set same no
                                      break
                                  }
                              }
                          }
                          if {$params(-detail) == "min" && !$same} {
                              lappend details -what attrs
                              lappend rlist [linsert $dt 0 -what "attrs c"]
                          }
                        } else {
                          lappend details -val1 $type1 -val2 $type2
                          lappend rlist [linsert $details 0 -what "type c"]
                        }
                      }
                      run {
                        # do nothing -- already handled above

                        # handled this comparison
                        set v2paths [lreplace $v2paths $i $i]
                      }
                      default {
                        error "bad part \"$part\": should be attrs, value, run"
                      }
                    }
                }
            }
        }

        # add any values left over in the obj2
        foreach path $v2paths {
            set details [list -path $path -obj1 $obj1 -obj2 $obj2]
            lappend rlist [linsert $details 0 -what "value +"]
        }
    }
    return $rlist
}

# ----------------------------------------------------------------------
# USAGE: _buildFailure <output>
#
# Returns a new Rappture::library object that contains a copy of the
# original test with the given <output> and a failing status.  This
# is used to represent the result of a test that aborts without
# producing a valid run.xml file.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Test::_buildFailure {output} {
    set info "<?xml version=\"1.0\"?>\n[$_testobj xml]"
    set obj [Rappture::LibraryObj ::#auto $info]
    $obj remove test

    $obj put output.time [clock format [clock seconds]]
    $obj put output.status failed
    $obj put output.user $::tcl_platform(user)
    $obj put output.log $output

    return $obj
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
