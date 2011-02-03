#! /bin/sh
# ----------------------------------------------------------------------
#  RAPPTURE REGRESSION TESTER
#
#  This program will read a set of test xml files typically located in
#  a tool's "tests" subdirectory, and provide an interactive test suite.
#  The test xml files should contain a complete set of inputs and 
#  outputs for one run of an application.  In each test xml, a label 
#  must be located at the path test.label.  Test labels may be organized
#  hierarchically by using dots to separate components of the test label
#  (example: roomtemp.1eV).  A description may optionally be located at
#  the path test.description.  Input arguments are the path to the
#  tool.xml of the version being tested, and the path the the directory
#  containing a set of test xml files.  If the arguments are missing,
#  the program will attempt to locate them automatically.
#
#  USAGE: tester.tcl ?-tool tool.xml? ?-testdir tests?
# ======================================================================
#  AUTHOR:  Ben Rafferty, Purdue University
#  Copyright (c) 2010  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
#\
exec tclsh "$0" $*
# ----------------------------------------------------------------------
# wish executes everything from here on...

set testerdir [file dirname [file normalize [info script]]]
lappend auto_path $testerdir

package require Itk
package require Img
package require Rappture
package require RapptureGUI

option add *selectBackground #bfefff
option add *Tooltip.background white
option add *Editor.background white
option add *Gauge.textBackground white
option add *TemperatureGauge.textBackground white
option add *Switch.textBackground white
option add *Progress.barColor #ffffcc
option add *Balloon.titleBackground #6666cc
option add *Balloon.titleForeground white
option add *Balloon*Label.font -*-helvetica-medium-r-normal-*-12-*
option add *Balloon*Radiobutton.font -*-helvetica-medium-r-normal-*-12-*
option add *Balloon*Checkbutton.font -*-helvetica-medium-r-normal-*-12-*
option add *ResultSet.controlbarBackground #6666cc
option add *ResultSet.controlbarForeground white
option add *ResultSet.activeControlBackground #ccccff
option add *ResultSet.activeControlForeground black
option add *Radiodial.length 3i
option add *BugReport*banner*foreground white
option add *BugReport*banner*background #a9a9a9
option add *BugReport*banner*highlightBackground #a9a9a9
option add *BugReport*banner*font -*-helvetica-bold-r-normal-*-18-*

switch $tcl_platform(platform) {
    unix - windows {
        event add <<PopupMenu>> <ButtonPress-3>
    }
    macintosh {
        event add <<PopupMenu>> <Control-ButtonPress-1>
    }
}

# install a better bug handler
Rappture::bugreport::install

# fix the "grab" command to support a stack of grab windows
Rappture::grab::init

# add the local image directory onto the path
Rappture::icon foo  ;# forces auto-loading of Rappture::icon
set Rappture::icon::iconpath [linsert $Rappture::icon::iconpath 0 [file join $testerdir images]]


Rappture::getopts argv params {
    value -tool ""
    value -testdir ""
}

# If tool.xml and test directory locations are not given, try to find them.
if {$params(-tool) == ""} {
    if {[file isfile tool.xml]} {
        set params(-tool) tool.xml
    } elseif {[file isfile [file join rappture tool.xml]]} {
        set params(-tool) [file join rappture tool.xml]
    } else {
        puts "Cannot find tool.xml"
        exit 1
    }
} elseif {![file isfile $params(-tool)]} {
    puts "Tool \"$params(-tool)\" does not exist"
    exit 1
}

if {$params(-testdir) == ""} {
    set tooldir [file dirname $params(-tool)]
    if {[file isdirectory [file join $tooldir tests]]} {
        set params(-testdir) [file join $tooldir tests]
    } elseif {[file isdirectory [file join [file dirname $tooldir] tests]]} {
        set params(-testdir) [file join [file dirname $tooldir] tests]
    } else {
        puts "Cannot find test directory"
        exit 1
    }
} elseif {![file isdirectory $params(-testdir)]} {
    puts "Test directory \"$params(-testdir)\" does not exist"
    exit 1
}

set installdir [file dirname [file normalize $params(-tool)]]
set xmlobj [Rappture::library $params(-tool)]
set ToolObj [Rappture::Tool ::#auto $xmlobj $installdir]

# ----------------------------------------------------------------------
# INITIALIZE WINDOW
# ----------------------------------------------------------------------
wm title . "Rappture Regression Tester"
wm geometry . 800x500
Rappture::Panes .pw -orientation horizontal -sashcursor sb_h_double_arrow
pack .pw -expand yes -fill both

set win [.pw pane 0]
Rappture::Tester::TestTree $win.tree \
    -selectcommand tester_selection_changed
pack $win.tree -expand yes -fill both -padx 8 -pady 8

set win [.pw insert end -fraction 0.8]

# Frame for viewing tests
# ----------------------------------------------------------------------
frame $win.testview
button $win.testview.regoldenize -text "<< New golden standard" \
    -state disabled -command tester_regoldenize
pack $win.testview.regoldenize -side bottom -anchor w
Rappture::Tooltip::for $win.testview.regoldenize \
    "If this test result differs from the established test case, you would normally fix your tool to produce the correct result.  In some cases, however, your updated tool may be producing different, but correct, results.  In those cases, you can press this button to update the test itself to use the current output as the new golden standard for this test case."

Rappture::Tester::TestView $win.testview.overview \
    -runcommand tester_run
pack $win.testview.overview -side top -fill both -padx 8 -pady 8

frame $win.testview.details
label $win.testview.details.heading -text "Differences:"
pack $win.testview.details.heading -side top -anchor w
Rappture::Scroller $win.testview.details.scrl \
    -xscrollmode auto -yscrollmode auto
pack $win.testview.details.scrl -expand yes -fill both
Rappture::Tester::StatusList $win.testview.details.scrl.list \
    -selectcommand tester_diff_show
$win.testview.details.scrl contents $win.testview.details.scrl.list

# Frame for viewing running tests
# ----------------------------------------------------------------------
frame $win.testrun
label $win.testrun.title -text "Output from test run:" -anchor w
pack $win.testrun.title -side top -anchor w

button $win.testrun.abort -text "Abort"
pack $win.testrun.abort -side bottom -pady {8 0}

Rappture::Scroller $win.testrun.scrl -xscrollmode auto -yscrollmode auto
pack $win.testrun.scrl -expand yes -fill both
text $win.testrun.scrl.info -width 1 -height 1 -wrap none
$win.testrun.scrl contents $win.testrun.scrl.info

# Load all tests in the test directory
# ----------------------------------------------------------------------
set testtree [.pw pane 0].tree
foreach file [glob -nocomplain -directory $params(-testdir) *.xml] {
    set testobj [Rappture::Tester::Test ::#auto $ToolObj $file]
    $testtree add $testobj
}
$testtree component treeview open -recurse root

# ----------------------------------------------------------------------
# USAGE: tester_selection_changed
#
# Invoked automatically whenever the selection changes in the tree
# on the left.  Brings up a description of one or more selected tests
# on the right-hand side.
# ----------------------------------------------------------------------
proc tester_selection_changed {args} {
    set testtree [.pw pane 0].tree
    set rhs [.pw pane 1]
    set testview $rhs.testview
    set tests [$testtree curselection]

    # figure out what we should be showing on the right-hand side
    if {[llength $tests] > 0} {
        set status "?"
        foreach obj $tests {
            if {[$obj getResult] == "Running"} {
                set status "Running"
            }
        }
        if {$status == "Running"} {
            set detailwidget $rhs.testrun
        } else {
            set detailwidget $rhs.testview
        }
    } else {
        set detailwidget ""
    }

    # repack the right-hand side, if necessary
    if {$detailwidget ne [pack slaves $rhs]} {
        foreach win [pack slaves $rhs] {
            pack forget $win
        }
        if {$detailwidget ne ""} {
            pack $detailwidget -expand yes -fill both -padx 8 -pady 8
        }
    }

    if {[llength $tests] > 0} {
        eval $testview.overview show $tests
        if {[llength $tests] == 1 && [$tests getResult] eq "Fail"} {
            pack $testview.regoldenize -side bottom -anchor w
            $testview.regoldenize configure -state normal

            # build up a detailed list of diffs for this one test
            pack $testview.details -side bottom -expand yes -fill both

            set testobj [lindex $tests 0]
            $testview.details.scrl.list delete 0 end
            foreach {op path what v1 v2} [$testobj getDiffs] {
                switch -- [lindex $what 0] {
                  value {
                    set title "Output: [$testobj getTestInfo $path.about.label]"
                    set icon [Rappture::icon fail16]
                    switch -- $op {
                      - { set desc "Result is missing from current output" }
                      + { set desc "Result was not expected to appear" }
                      c { set desc "Result differs from expected value" }
                      default {
                          error "don't know how to handle difference $op"
                      }
                    }
                  }
                  structure {
                    set ppath [lindex $what 1]
                    set title "Output: [$testobj getTestInfo $ppath.about.label]"
                    set icon [Rappture::icon warn16]
                    set pplen [string length $ppath]
                    set tail [string range $path [expr {$pplen+1}] end]
                    switch -- $op {
                      - { set desc "Missing value \"$v1\" at $tail" }
                      + { set desc "Extra value \"$v2\" at $tail" }
                      c { set desc "Details at $tail have changed:\n       got: $v2\n  expected: $v1" }
                      default {
                          error "don't know how to handle difference $op"
                      }
                    }
                  }
                  default {
                    error "don't know how to handle difference \"$what\""
                  }
                }

                # add to the list of differences
                $testview.details.scrl.list insert end \
                    -title $title -subtitle $path -body $desc \
                    -icon $icon -clientdata $testobj
            }

        } else {
            $testview.regoldenize configure -state disabled
            pack forget $testview.details $testview.regoldenize
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: tester_run <testObj> <testObj> ...
#
# Invoked whenever the user presses the "Run" button for one or more
# selected tests.  Puts the tool into "run" mode and starts running
# the various test cases.
# ----------------------------------------------------------------------
proc tester_run {args} {
    # set up a callback for handling output from runs
    Rappture::Tester::Test::queue status tester_run_output

    # add these tests to the run queue
    eval Rappture::Tester::Test::queue add $args

    # show the run output window
    set rhs [.pw pane 1]
    foreach win [pack slaves $rhs] {
        pack forget $win
    }
    pack $rhs.testrun -expand yes -fill both -padx 8 -pady 8
}

# ----------------------------------------------------------------------
# USAGE: tester_run_output start <testObj>
# USAGE: tester_run_output add <testObj> <string>
#
# Handles the output from running tests.  The "start" option clears
# the current output area.  The "add" option adds output from a run.
# ----------------------------------------------------------------------
proc tester_run_output {option testobj args} {
    set testrun [.pw pane 1].testrun

    switch -- $option {
        start {
            # clear out any previous output
            $testrun.scrl.info configure -state normal
            $testrun.scrl.info delete 1.0 end
            $testrun.scrl.info configure -state disabled

            # plug this object into the "Abort" button
            $testrun.abort configure -command [list $testobj abort]
        }
        add {
            $testrun.scrl.info configure -state normal
            $testrun.scrl.info insert end [lindex $args 0]

            # if there are too many lines, delete some
            set lines [lindex [split [$testrun.scrl.info index end-2char] .] 0]
            if {$lines > 500} {
                set extra [expr {$lines-500+1}]
                $testrun.scrl.info delete 1.0 $extra.0
            }

            # show the newest stuff
            $testrun.scrl.info see end
            $testrun.scrl.info configure -state disabled
        }
        default {
            error "bad option \"$option\": should be start, add"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: tester_diff_show -option value -option value ...
#
# Pops up a panel showing more detailed information about a particular
# difference found in a particular test case.
# ----------------------------------------------------------------------
proc tester_diff_show {args} {
    puts "SHOW DETAIL: $args"
}

# ----------------------------------------------------------------------
# USAGE: tester_regoldenize
#
# Regoldenizes the currently focused test case.  Displays a warning
# message.  If confirmed, copy the test information from the existing
# test xml into the new result, and write the new data into the test
# xml.
# ----------------------------------------------------------------------
proc tester_regoldenize {} {
    set testtree [.pw pane 0].tree

    set test [$testtree getTest]
    set testxml [$test getTestxml]
    if {[tk_messageBox -type yesno -icon warning -message "Are you sure you want to regoldenize?\n$testxml will be overwritten."]} {
        $test regoldenize

        # reload the updated description for this test
        tester_selection_changed
    }
}
