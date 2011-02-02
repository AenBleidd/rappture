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

    public method run {args}
    public method regoldenize {}

    private variable _toolobj ""  ;# Rappture::Tool for tool being tested
    private variable _testxml ""  ;# XML file for test case
    private variable _testobj ""  ;# Rappture::Library object for _testxml

    private variable _added ""
    private variable _diffs ""
    private variable _missing ""
    private variable _result "?"
    private variable _runobj ""

    # don't need this?
    public method getAdded {}
    public method getDiffs {}
    public method getInputs {{path input}}
    public method getMissing {}
    public method getOutputs {{path output}}
    public method getRunobj {}
    public method getTestobj {}

    private method _setResult {name}
    private method added {lib1 lib2 {path output}}
    private method compareElements {lib1 lib2 path}
    private method diffs {lib1 lib2 {path output}}
    private method merge {toolobj golden driver {path input}}
    private method missing {lib1 lib2 {path output}}
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
    }

    # copy inputs from the test into the run file
    foreach path [Rappture::entities -as path $_testobj input] {
        if {[$_testobj element -as type $path.current] ne ""} {
puts "  override: $path = [$_testobj get $path.current]"
            lappend args $path [$_testobj get $path.current]
        }
    }

    # run the test case...
    _setResult "Running"
    foreach {status _runobj} [eval $_toolobj run $args] break

    if {$status == 0 && [Rappture::library isvalid $_runobj]} {
        # HACK: Add a new input to differentiate between results
        $_runobj put input.TestRun.current "Test result"
        set _diffs [diffs $_testobj $_runobj]
        set _missing [missing $_testobj $_runobj]
        set _added [added $_testobj $_runobj]
        if {$_diffs == "" && $_missing == "" && $_added == ""} {
            _setResult "Pass"
        } else {
            _setResult "Fail"
        }
    } else {
        set _runobj ""
        set _setResult "Error"
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
        error "Test has not yet been run."
    }
    $_runobj put test.label [$_testobj get test.label]
    $_runobj put test.description [$_testobj get test.description]
    set fid [open $_testxml w]
    puts $fid "<?xml version=\"1.0\"?>"
    puts $fid [$_runobj xml]
    close $fid
    set _testobj $_runobj
    set _diffs ""
    set _added ""
    set _missing ""
    _setResult Pass
}

# ----------------------------------------------------------------------
# USAGE: getAdded
#
# Return a list of paths that have been added that do not exist in the
# golden results.  Throws an error if the test has not been ran.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Test::getAdded {} {
    if {$_runobj eq ""} {
        error "Test has not yet been run."
    }
    return $_added
}

# ----------------------------------------------------------------------
# USAGE: getDiffs
#
# Returns a list of paths that exist in both the golden and new results,
# but contain data that does not match according to the compareElements
# method.  Throws an error if the test has not been run.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Test::getDiffs {} {
    return [list \
        input.number(temperature) label \
        output.curve(f12) units \
        output.curve(f12) result]
}

# -----------------------------------------------------------------------
# USAGE: getInputs
#
# Returns a list of key value pairs for all inputs given in the test xml.
# Each key is the path to the input element, and each key is its current
# value. 
# -----------------------------------------------------------------------
itcl::body Rappture::Tester::Test::getInputs {{path input}} {
    set retval [list]
    foreach child [$_testobj children $path] {
        set fullpath $path.$child
        if {$fullpath != "input.TestRun"} {
            set val [$_testobj get $fullpath.current]
            if {$val != ""} {
                lappend retval $fullpath $val
            }
        }
        append retval [getInputs $fullpath]
    }
    return $retval
}

# ----------------------------------------------------------------------
# USAGE: getMissing
#
# Return a list of paths that are present in the golden results, but are
# missing in the new test results.  Throws an error if the test has not
# been ran.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Test::getMissing {} {
    if {$_runobj eq ""} {
        error "Test has not yet been run."
    }
    return $_missing
}

# ----------------------------------------------------------------------
# USAGE: getOutputs
#
# Returns a list of key value pairs for all outputs in the runfile
# generated by the last run of the test.  Each key is the path to the
# element, and each value is its status (ok, diff, added, or missing).
# Throws an error if the test has not been run.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Test::getOutputs {{path output}} {
    if {$_runobj eq ""} {
        error "Test has not yet been run."
    }
    set retval [list]
    foreach child [$_runobj children $path] {
        set fullpath $path.$child
        if {$fullpath != "output.time" && $fullpath != "output.user" \
            && $fullpath != "output.status"} {
            if {[lsearch $fullpath [getDiffs]] != -1} {
                set status diff
            } elseif {[lsearch $fullpath [getAdded]] != -1} {
                set status added
            } else {
                if {[$_runobj get $fullpath] != ""} {
                    set status ok
                } else {
                    set status ""
                }
            }
            lappend retval $fullpath $status
        }
        append retval " [getOutputs $fullpath]"
    }
    # We won't find missing elements by searching through runobj.  Instead,
    # tack on all missing items at the end (only do this once)
    if {$path == "output"} {
        foreach item $_missing {
            lappend retval $item missing
        }
    }
    return $retval
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
# Returns the test library object containing the set of golden results.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Test::getTestobj {} {
    return $_testobj
}

# ----------------------------------------------------------------------
# USAGE: added lib1 lib2 ?path?
#
# Compares two library objects and returns a list of paths that have
# been added in the second library and do not exist in the first.
# Return value will contain all differences that occur as descendants of
# an optional starting path.  If the path argument is not given, then 
# only the output sections will be compared.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Test::added {lib1 lib2 {path output}} {
    set paths [list]
    foreach child [$lib2 children $path] {
        foreach p [added $lib1 $lib2 $path.$child] {
            lappend paths $p
        }
    }
    if {[$lib1 get $path] == "" && [$lib2 get $path] != ""} {
        lappend paths $path
    }
    return $paths
}

# ----------------------------------------------------------------------
# USAGE: compareElements <lib1> <lib2> <path>
#
# Compare data found in two library objects at the given path.  Returns
# 1 if match, 0 if no match.  For now, just check if ascii identical.
# Later, we can do something more sophisticated for different types of 
# elements.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Test::compareElements {lib1 lib2 path} {
    set val1 [$lib1 get $path]
    set val2 [$lib2 get $path]
    return [expr {$val1} != {$val2}]
}

# ----------------------------------------------------------------------
# USAGE: diffs <lib1> <lib2> ?path?
#
# Compares two library objects and returns a list of paths that do not
# match.  Only paths which exist in both libraries are considered.
# Return value will contain all differences that occur as descendants of
# an optional starting path.  If the path argument is not given, then 
# only the output sections will be compared.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Test::diffs {lib1 lib2 {path output}} {
    set paths [list]
    set clist1 [$lib1 children $path]
    set clist2 [$lib2 children $path]
    foreach child $clist1 {
        # Ignore if not present in both libraries
        if {[lsearch -exact $clist2 $child] != -1} {
            foreach p [diffs $lib1 $lib2 $path.$child] {
                lappend paths $p
            }
        }
    }
    if {[compareElements $lib1 $lib2 $path]} {
        # Ignore output.time and output.user
        if {$path != "output.time" && $path != "output.user"} {
            lappend paths $path
        }
    }
    return $paths
}

# ----------------------------------------------------------------------
# USAGE: merge <toolobj> <golden> <driver> ?path?
#
# Used to recursively build up a driver library object for running a
# test.  Should not be called directly - see makeDriver.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Test::merge {toolobj golden driver {path input}} {
    foreach child [$toolobj children $path] {
        set val [$golden get $path.$child.current]
        if {$val != ""} {
            $driver put $path.$child.current $val
        } else {
            set def [$toolobj get $path.$child.default]
            if {$def != ""} {
                $driver put $path.$child.current $def
            }
        }
        merge $toolobj $golden $driver $path.$child
    }
    return $driver
}

# ----------------------------------------------------------------------
# USAGE: added lib1 lib2 ?path?
#
# Compares two library objects and returns a list of paths that do not
# exist in the first library and have been added in the second.
# Return value will contain all differences that occur as descendants of
# an optional starting path.  If the path argument is not given, then 
# only the output sections will be compared.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Test::missing {lib1 lib2 {path output}} {
    set paths [list]
    foreach child [$lib1 children $path] {
        foreach p [missing $lib1 $lib2 $path.$child] {
            lappend paths $p
        }
    }
    if {[$lib1 get $path] != "" && [$lib2 get $path] == ""} {
        lappend paths $path
    }
    return $paths
}

# ----------------------------------------------------------------------
# USAGE: _setResult ?|Pass|Fail|Waiting|Running
#
# Used internally to change the state of this test case.  If there
# is a -notifycommand script for this object, it is invoked to notify
# an interested client that the object has changed.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Test::_setResult {name} {
puts "CHANGED: $this => $name"
    set _result $name
    if {[string length $notifycommand] > 0} {
puts "  notified $notifycommand"
        uplevel #0 $notifycommand $this
    }
}
