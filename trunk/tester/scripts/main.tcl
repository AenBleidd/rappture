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
#  AUTHORS:  Michael McLennan, Ben Rafferty, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
#\
exec tclsh "$0" $*
# ----------------------------------------------------------------------
# wish executes everything from here on...

# take the main window down for now, so we can avoid a flash on the screen
wm withdraw .

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
option add *Diffview.background white
option add *Text.background white
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

option add *testdiffs.hd*background #666666
option add *testdiffs.hd*highlightBackground #666666
option add *testdiffs.hd*foreground white
option add *testdiffs.hd.inner.highlightBackground #999999
option add *testdiffs.hd.inner*font {Arial -12 bold}
option add *testdiffs.hd.inner*help.font {Arial -10 italic}
option add *testdiffs.hd.inner*help.padX 2
option add *testdiffs.hd.inner*help.padY 2
option add *testdiffs.hd.inner*help.borderWidth 1
option add *testdiffs.hd.inner*help.relief flat
option add *testdiffs.hd.inner*help.overRelief raised
option add *testdiffs.legend*font {Arial -12}

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

# bring in the Rappture object system
Rappture::objects::init

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
set TaskObj [Rappture::Task ::#auto $xmlobj $installdir]

# tasks in the tester run quietly and discard results
$TaskObj configure -jobstats "" -resultdir ""

set DiffShow ""  ;# used to track which diff objects are being displayed

# ----------------------------------------------------------------------
# INITIALIZE WINDOW
# ----------------------------------------------------------------------
wm title . "Rappture Regression Tester"
wm geometry . 800x500
Rappture::Panes .pw -orientation horizontal
pack .pw -expand yes -fill both

set win [.pw pane 0]
Rappture::Tester::TestTree $win.tree \
    -selectcommand tester_selection_changed
pack $win.tree -expand yes -fill both -padx 8 -pady 8

set win [.pw insert end -fraction 0.8]

# Frame for viewing tests
# ----------------------------------------------------------------------
frame $win.testview
frame $win.testview.bbar
pack $win.testview.bbar -side bottom -fill x -pady {8 0}
button $win.testview.bbar.regoldenize -text "<< New golden standard" \
    -state disabled -command tester_regoldenize
pack $win.testview.bbar.regoldenize -side left
Rappture::Tooltip::for $win.testview.bbar.regoldenize \
    "If this test result differs from the established test case, you would normally fix your tool to produce the correct result.  In some cases, however, your updated tool may be producing different, but correct, results.  In those cases, you can press this button to update the test itself to use the current output as the new golden standard for this test case."

pack $win.testview.bbar -side bottom -fill x

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
    -viewcommand tester_diff_show
$win.testview.details.scrl contents $win.testview.details.scrl.list

# Frame for viewing running tests
# ----------------------------------------------------------------------
frame $win.testrun
label $win.testrun.title -text "Output from test run:" -anchor w
pack $win.testrun.title -side top -anchor w

Rappture::Progress $win.testrun.progress
button $win.testrun.abort -text "Abort"
pack $win.testrun.abort -side bottom -pady {8 0}

Rappture::Scroller $win.testrun.scrl -xscrollmode auto -yscrollmode auto
pack $win.testrun.scrl -expand yes -fill both
text $win.testrun.scrl.info -width 1 -height 1 -wrap none
$win.testrun.scrl contents $win.testrun.scrl.info

# Frame for viewing diffs
# ---------------------------------------------------------------------
frame .testdiffs -borderwidth 10 -relief flat

# header at the top with info about the diff, help, and close button
frame .testdiffs.hd -borderwidth 4 -relief flat
pack .testdiffs.hd -side top -fill x
frame .testdiffs.hd.inner -highlightthickness 1 -borderwidth 2 -relief flat
pack .testdiffs.hd.inner -expand yes -fill both
button .testdiffs.hd.inner.close -relief flat -overrelief raised \
    -bitmap [Rappture::icon dismiss] -command tester_diff_hide
pack .testdiffs.hd.inner.close -side right -padx 8
label .testdiffs.hd.inner.title -compound left -anchor w -padx 8
pack .testdiffs.hd.inner.title -side left
button .testdiffs.hd.inner.help -anchor w -text "Help..." \
    -command "::Rappture::Tooltip::tooltip show .testdiffs.hd.inner.help +10,0"
pack .testdiffs.hd.inner.help -side left -padx 10

# show add/deleted styles at the bottom
Rappture::Tester::Legend .testdiffs.legend
pack .testdiffs.legend -side bottom -fill x
frame .testdiffs.line -height 1 -background black
pack .testdiffs.line -side bottom -fill x -pady {0 2}

# diff viewer goes in this spot
frame .testdiffs.body
pack .testdiffs.body -expand yes -fill both -padx 10 -pady {20 10}

# viewer for attribute diffs
Rappture::Tester::ObjView .testdiffs.body.attrs

# viewer for run status diffs
Rappture::Tester::RunView .testdiffs.body.runs

# viewer for value diffs where object is extra or missing
frame .testdiffs.body.val
Rappture::Tester::ObjView .testdiffs.body.val.obj -details max -showdiffs no
pack .testdiffs.body.val.obj -expand yes -fill both

# viewer for value diffs where we have just one string
frame .testdiffs.body.val1str
Rappture::Tester::ObjView .testdiffs.body.val1str.obj \
    -details min -showdiffs no
pack .testdiffs.body.val1str.obj -side top -fill x
label .testdiffs.body.val1str.l -text "Value:"
pack .testdiffs.body.val1str.l -anchor w -padx 10 -pady {10 0}
Rappture::Scroller .testdiffs.body.val1str.scrl \
    -xscrollmode auto -yscrollmode auto
pack .testdiffs.body.val1str.scrl -expand yes -fill both -padx 10 -pady {0 10}
text .testdiffs.body.val1str.scrl.text -width 10 -height 1 -wrap char
.testdiffs.body.val1str.scrl contents .testdiffs.body.val1str.scrl.text

# viewer for value diffs where we have two strings but no special viewers
frame .testdiffs.body.val2strs
Rappture::Tester::ObjView .testdiffs.body.val2strs.obj \
    -details min -showdiffs no
pack .testdiffs.body.val2strs.obj -side top -fill x
Rappture::Tester::StringDiffs .testdiffs.body.val2strs.diffs \
    -title1 "Expected this:" -title2 "Got this:"
pack .testdiffs.body.val2strs.diffs -expand yes -fill both -padx 10 -pady 10

# viewer for value diffs where we have a special object viewer
Rappture::Panes .testdiffs.body.val2objs -orientation horizontal

# empty area for the object value viewer
set win [.testdiffs.body.val2objs pane 0]
frame $win.val
pack $win.val -expand yes -fill both

# show object details and diff on the right-hand side
set win [.testdiffs.body.val2objs insert end -fraction 0.5]
Rappture::Tester::ObjView $win.obj -details min -showdiffs no
pack $win.obj -side top -fill x
Rappture::Tester::StringDiffs $win.diffs \
    -title1 "Expected this:" -title2 "Got this:"
pack $win.diffs -expand yes -fill both -padx 4 -pady 4

# viewer for showing string info if all else fails (e.g. unkobj errors)
Rappture::Scroller .testdiffs.body.info \
    -xscrollmode auto -yscrollmode auto
pack .testdiffs.body.info -expand yes -fill both -padx 10 -pady {0 10}
text .testdiffs.body.info.text -width 10 -height 1 -wrap char
.testdiffs.body.info contents .testdiffs.body.info.text

# plug the proper diff colors into the legend area
.testdiffs.legend insert end -title "= Added" -shape box \
    -color [.testdiffs.body.val2strs.diffs cget -addedbackground]
.testdiffs.legend insert end -title "= Deleted" -shape box \
    -color [.testdiffs.body.val2strs.diffs cget -deletedbackground]
.testdiffs.legend insert end -title "= Changed" -shape box \
    -color [.testdiffs.body.val2strs.diffs cget -changedbackground]
.testdiffs.legend insert end -title "= Test Result" -shape line \
    -color red -anchor e
.testdiffs.legend insert end -title "= Expected Result" -shape line \
    -color black -anchor e

# Load all tests in the test directory
# ----------------------------------------------------------------------
set testtree [.pw pane 0].tree
foreach file [glob -nocomplain -directory $params(-testdir) *.xml] {
    set testobj [Rappture::Tester::Test ::#auto $TaskObj $file]
    if {[$testobj getTestInfo test.label] eq ""} {
        puts stderr "ERROR:  Missing test label in $file"
        puts stderr "        skipping that test..."
    } elseif {[catch {$testtree add $testobj} err]} {
        puts stderr "ERROR:  Can't load test $file"
        puts stderr "        $err"
        puts stderr "        skipping that test..."
    }
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
            pack $testview.bbar.regoldenize -side left
            $testview.bbar.regoldenize configure -state normal

            # build up a detailed list of diffs for this one test
            pack $testview.details -side bottom -expand yes -fill both

            set testobj [lindex $tests 0]
            $testview.details.scrl.list delete 0 end
            foreach rec [$testobj getDiffs] {
                catch {unset diff}
                array set diff $rec

                set section [string totitle [lindex [split $diff(-path) .] 0]]
                set title "$section: [$testobj getTestInfo $diff(-path).about.label]"
                set desc ""
                set help ""

                set difftype [lindex $diff(-what) 0]
                set op [lindex $diff(-what) 1]
                switch -- $difftype {
                  value {
                    if {$section eq "Output"} {
                        set icon [Rappture::icon fail16]
                        switch -- $op {
                          - {
                              set desc "Result is missing from current output"
                              set help "This result was defined in the test case, but was missing from the output from the current test run.  Perhaps the tool is not producing the result as it should, or else the latest version of the tool no longer produces that result and the test case needs to be updated."
                          }
                          + {
                              set desc "Result was not expected to appear"
                              set help "The test run contained a result that was not part of the expected output.  Perhaps the tool is not supposed to produce that result, or else the latest version produces a new result and the test case needs to be updated."
                          }
                          c {
                              set desc "Result differs from expected value"
                              set help "The result from the test run doesn't match the expected result in the test case.  The tool should be fixed to produce the expected result.  If you can verify that the tool is working correctly, then the test case should be updated to contain this new result."
                          }
                          default {
                            error "don't know how to handle difference $op"
                          }
                        }
                    } elseif {$section eq "Input"} {
                        set icon [Rappture::icon warn16]
                        switch -- $op {
                          - {
                              set desc "Test case doesn't specify this input value"
                              set help "The test case is missing a setting for this input value that appears in the current tool definition.  Is this a new input that was recently added to the tool?  If so, the test case should be updated."
                          }
                          + {
                              set desc "Test case has this extra input value"
                              set help "The test case has an extra input value that does not appear in the current tool definition.  Was this input recently removed from the tool?  If so, the test case should be updated."
                          }
                          c {
                              # don't give a warning in this case
                              # input is supposed to be different from tool.xml
                          }
                          default {
                            error "don't know how to handle difference $op"
                          }
                        }
                    }
                  }
                  attrs {
                    if {$section eq "Output"} {
                        set icon [Rappture::icon warn16]
                        set desc "Details about this result have changed"
                        set help "The test run produced an output with slightly different information.  This may be as simple as a change in the label or description, or as serious as a change in the physical system of units.  Perhaps the tool is producing the wrong output, or else the tool has been modified and the test case needs to be updated."
                    } elseif {$section eq "Input"} {
                        set icon [Rappture::icon warn16]
                        set desc "Details about this result have changed"
                        set help "The test run contains an input with slightly different information.  This may be as simple as a change in the label or description, or as serious as a change in the physical system of units.  Perhaps this input has been modified in the latest version of the tool and the test case is outdated."
                    }
                  }
                  type {
                    if {$section eq "Output"} {
                        set icon [Rappture::icon fail16]
                        set desc "Result has the wrong type"
                        set help "The test run contains an output that is completely different from what was expected--not even the same type of object.  The tool should be fixed to produce the expected result.  If you can verify that the tool is working correctly, then the test case should be updated to contain this new result."
                    } elseif {$section eq "Input"} {
                        set icon [Rappture::icon warn16]
                        set desc "Input value has a different type"
                        set help "The test run contains an output that is completely different from what was expected--not even the same type of object.  The tool should be fixed to produce the expected result.  If you can verify that the tool is working correctly, then the test case should be updated to contain this new result."
                        set help "The test run contains an input value that is completely different from the corresponding input defined in the test case.  Was this input recently modified in the tool?  If so, the test case should be updated."
                    }
                  }
                  status {
                    set icon [Rappture::icon fail16]
                    set title "Run failure"
                    if {$diff(-path) eq "output.status"} {
                        set desc "Test run failure was not expected"
                        set help "The test run failed, but the test case was expected to finish successfully.  The tool should be fixed to avoid the failure.  If you can verify that the tool is working correctly, then the test case should be updated to contain this new result."
                    } else {
                        set desc "Test run failure produced different output"
                        set help "The test run failed as expected, but produced different output.  Fix the tool to produce the correct error message for the failure.  If you can verify that the latest error message is better, then the test case should be updated to contain this new output."
                    }
                  }
                  unkobj {
                    set icon [Rappture::icon warn16]
                    set title "Unsupported object type"
                    set desc "Test contains an object type that is not yet supported"
                    set help "The Rappture Regression Tester is still in beta.  Some object types supported by the Rappture runtime environment are not yet supported by the tester.  These warnings will go away as the Rappture Regression Tester improves and support for all object types is put into place."
                  }
                  default {
                    error "don't know how to handle difference \"$difftype\""
                  }
                }

                # add to the list of differences
                if {$desc ne ""} {
                    $testview.details.scrl.list insert end \
                        -title $title -subtitle $diff(-path) \
                        -icon $icon -body $desc -help $help \
                        -clientdata [linsert $rec 0 -testobj $testobj]
                }
            }

        } else {
            $testview.bbar.regoldenize configure -state disabled
            pack forget $testview.details $testview.bbar.regoldenize
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

            # hide progress bar
            pack forget $testrun.progress
        }
        add {
            set message [lindex $args 0]
            # scan for progress updates
            while {[regexp -indices \
                {=RAPPTURE-PROGRESS=> *([-+]?[0-9]+) +([^\n]*)(\n|$)} $message \
                 match percent mesg]} {

                foreach {i0 i1} $percent break
                set percent [string range $message $i0 $i1]

                foreach {i0 i1} $mesg break
                set mesg [string range $message $i0 $i1]

                pack $testrun.progress -fill x -padx 10 -pady 10
                $testrun.progress settings -percent $percent -message $mesg

                foreach {i0 i1} $match break
                set message [string replace $message $i0 $i1]
            }

            $testrun.scrl.info configure -state normal
            $testrun.scrl.info insert end $message

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
    global DiffShow

    set testtree [.pw pane 0].tree
    set rhs [.pw pane 1]
    set testview $rhs.testview
    set viewarea [.testdiffs.body.val2objs pane 0].val

    # clean up from the last call
    set viewer [lindex $DiffShow 0]
    if {$viewer ne ""} {
        $viewer delete
        foreach obj [lrange $DiffShow 1 end] {
            itcl::delete object $obj
        }
    }
    set DiffShow ""

    # show the diff overview in the header
    array set data $args
    .testdiffs.hd.inner.title configure -image $data(-icon) -text $data(-body)

    #
    # Figure out how to visualize the difference.
    #
    array set diff $data(-clientdata)

    if {[info exists data(-help)]} {
        Rappture::Tooltip::text .testdiffs.hd.inner.help $data(-help)
    } else {
        Rappture::Tooltip::text .testdiffs.hd.inner.help ""
    }

    switch -glob -- $diff(-what) {
        "value *" {
            foreach w [pack slaves $viewarea] {
                pack forget $w
            }

            set op [lindex $diff(-what) 1]
            set w .testdiffs.body.val1str.obj
            switch -- $op {
                +       { set bg [$w cget -addedbackground] }
                -       { set bg [$w cget -deletedbackground] }
                default { set bg [lindex [$w configure -background] 3] }
            }

            # get the first value in obj or string form
            set val1 ""
            if {[catch {Rappture::objects::import $diff(-obj1) $diff(-path)} val1] == 0 && $val1 ne ""} {
                set viewer [Rappture::objects::viewer $val1 \
                    -for output -parent $viewarea]
                if {$viewer ne ""} {
                    set DiffShow [list $viewer $val1]
                }
                # try to get a string rep too
                set status [$val1 export string str]
                if {[lindex $status 0]} { set vstr1 $str }
            }

            # get a string rep for the second value
            set val2 ""
            if {[catch {Rappture::objects::import $diff(-obj2) $diff(-path)} val2] == 0 && $val2 ne ""} {
                set viewer [Rappture::objects::viewer $val2 \
                    -for output -parent $viewarea]
                if {$viewer ne ""} {
                    if {$DiffShow ne ""} {
                        if {[lindex $DiffShow 0] eq $viewer} {
                            lappend DiffShow $val2
                        } else {
                            error "type mismatch between values: $diff(-obj1) vs $diff(-obj2) -- diff should have caught this as \"type\" difference"
                        }
                    } else {
                        set DiffShow [list $viewer $val2]
                    }
                }
                # try to get a string rep too
                set status [$val2 export string str]
                if {[lindex $status 0]} { set vstr2 $str }
            }

            if {$DiffShow ne ""} {
                # we have a value viewer -- show the values in that
                pack $viewer -expand yes -fill both

                if {$val1 ne ""} {
                    $viewer add $val1 [list -color black -description "Expected Result"]
                }
                if {$val2 ne ""} {
                    $viewer add $val2 [list -color red -description "Test Result"]
                }

                # show the string diffs too
                set w [.testdiffs.body.val2objs pane 1]
                $w.obj configure -background $bg \
                    -testobj $diff(-testobj) -path $diff(-path)

                if {[info exists vstr1]} { set v1 $vstr1 } else { set v1 "" }
                if {[info exists vstr2]} { set v2 $vstr2 } else { set v2 "" }
                $w.diffs show $v1 $v2

                set win .testdiffs.body.val2objs

                set legsettings [list 2 normal \
                    3 [expr {($val2 ne "") ? "normal" : "disabled"}] \
                    4 [expr {($val1 ne "") ? "normal" : "disabled"}]]

            } elseif {[info exists vstr1] && [info exists vstr2]} {
                # we have two value strings -- show as a diff
                set win .testdiffs.body.val2strs
                $win.obj configure -background $bg \
                    -testobj $diff(-testobj) -path $diff(-path)
                $win.diffs show $vstr1 $vstr2
                set legsettings {2 normal 3 disabled 4 disabled}
            } elseif {[info exists vstr1] || [info exists vstr2]} {
                # we have one value string -- show it
                if {[info exists vstr1]} {
                    set val $vstr1
                } else {
                    set val $vstr2
                }
                set win .testdiffs.body.val1str
                $win.obj configure -background $bg \
                    -testobj $diff(-testobj) -path $diff(-path)
                $win.scrl.text configure -state normal
                $win.scrl.text delete 1.0 end
                $win.scrl.text insert end $val
                $win.scrl.text configure -state disabled
                set legsettings {2 disabled 3 disabled 4 disabled}
            } else {
                # don't have a value -- show the attributes
                set win .testdiffs.body.val
                $win.obj configure -background $bg \
                    -testobj $diff(-testobj) -path $diff(-path)
                set legsettings {2 disabled 3 disabled 4 disabled}
            }

            # clean up any objects that are not being stored
            if {$val1 ne "" && [lsearch $DiffShow $val1] < 0} {
                itcl::delete object $val1
            }
            if {$val2 ne "" && [lsearch $DiffShow $val2] < 0} {
                itcl::delete object $val2
            }
        }
        "attrs *" {
            set win .testdiffs.body.attrs
            set bg [lindex [$win configure -background] 3]
            $win configure -testobj $diff(-testobj) -background $bg \
                -path $diff(-path) -details max -showdiffs yes
            set legsettings {2 disabled 3 disabled 4 disabled}
        }
        "status" {
            set win .testdiffs.body.runs
            $win configure -testobj $diff(-testobj) -showdiffs yes
            set legsettings {2 disabled 3 disabled 4 disabled}
        }
        "type *" {
            error "don't know how to show type diffs"
            set legsettings {2 disabled 3 disabled 4 disabled}
        }
        "unkobj" {
            set win .testdiffs.body.info
            $win.text configure -state normal
            $win.text delete 1.0 end
            $win.text insert end "The following object type is not yet supported within the Rappture Regression Tester:\n\n[$diff(-testobj) getTestInfo xml $diff(-path)]"
            $win.text configure -state disabled
            set legsettings {2 disabled 3 disabled 4 disabled}
        }
    }
    if {[pack slaves .testdiffs.body] ne $win} {
        foreach w [pack slaves .testdiffs.body] {
            pack forget $w
        }
        pack $win -expand yes -fill both
    }

    # fix up the legend to best explain the current result
    foreach {index state} $legsettings {
        .testdiffs.legend itemconfigure $index -state $state
    }

    # pop up the viewer
    place .testdiffs -x 0 -y 0 -anchor nw -relwidth 1 -relheight 1
    raise .testdiffs
}

# ----------------------------------------------------------------------
# USAGE: tester_diff_hide
#
# Takes down the panel posted by tester_diff_show.
# ----------------------------------------------------------------------
proc tester_diff_hide {} {
    place forget .testdiffs
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
    set seltests [$testtree curselection]

    if {[llength $seltests] != 1} {
        error "Oops! Multiple tests selected to regoldenize.  How did we get here?"
    }

    set test [lindex $seltests 0]
    set testxml [$test getTestxml]
    if {[tk_messageBox -type yesno -icon warning -title "Rappture Tester: Confirm" -message "Are you sure that the latest run results are completely correct for this test case?  If so, then $testxml will be overwritten with the latest output, and all future test runs will be compared with this new standard.\n\nContinue?"]} {
        $test regoldenize

        # reload the updated description for this test
        tester_selection_changed
    }
}

wm deiconify .
