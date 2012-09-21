# ----------------------------------------------------------------------
#  COMPONENT: testview - display the results of a test
#
#  Top part of right hand side of the regression tester.  Displays an
#  overview of selected test cases, and offers a button for running
#  them.
# ======================================================================
#  AUTHOR:  Ben Rafferty, Purdue University
#           Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require BLT 

namespace eval Rappture::Tester::TestView { #forward declaration }

option add *TestView.font {Arial -12} widgetDefault
option add *TestView.titleFont {Arial -18 bold} widgetDefault
option add *TestView.statusFont {Arial -12 italic} widgetDefault
option add *TestView.statusPassColor black widgetDefault
option add *TestView.statusFailColor red widgetDefault
option add *TestView*descScroller.width 3i widgetDefault
option add *TestView*descScroller.height 1i widgetDefault

itcl::class Rappture::Tester::TestView {
    inherit itk::Widget 

    itk_option define -statuspasscolor statusPassColor StatusColor ""
    itk_option define -statusfailcolor statusFailColor StatusColor ""
    itk_option define -runcommand runCommand RunCommand ""

    constructor {args} { #defined later }
    public method show {args}

    private method _doRun {}
    private method _plural {n}

    private variable _testobjs ""  ;# objects being displayed
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestView::constructor {args} {

    # run button to run selected tests
    itk_component add run {
        button $itk_interior.run -text "Run" -padx 12 -pady 12 \
            -command [itcl::code $this _doRun]
    }
    pack $itk_component(run) -side right -anchor n

    # shows the big icon for test: pass/fail
    itk_component add statusicon {
        label $itk_interior.sicon
    }
    pack $itk_component(statusicon) -side left -anchor n

    # shows the name of the test
    itk_component add title {
        label $itk_interior.title -anchor w
    } {
        usual
        rename -font -titlefont titleFont Font
    }
    pack $itk_component(title) -side top -anchor w

    # shows the status of the test: pass/fail
    itk_component add statusmesg {
        label $itk_interior.smesg -anchor w
    } {
        usual
        rename -font -statusfont statusFont Font
        ignore -foreground
    }
    pack $itk_component(statusmesg) -side top -anchor w

    # for the longer text describing the test
    itk_component add descScroller {
        Rappture::Scroller $itk_interior.descScroller \
            -xscrollmode auto -yscrollmode auto
    }
    pack $itk_component(descScroller) -expand yes -fill both

    itk_component add description {
        text $itk_component(descScroller).desc -height 1 \
            -wrap word -relief flat
    }
    $itk_component(descScroller) contents $itk_component(description)

    eval itk_initialize $args
}

itk::usual TestView {
    keep -background -foreground -cursor
    keep -font -titlefont -statusfont
}

# ----------------------------------------------------------------------
# USAGE: show <testObj> <testObj> ...
#
# Loads one or more Test objects into the display.  When a single
# object is shown, we can display more detail.  When several objects
# are shown, we provide overview info.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestView::show {args} {
    foreach obj $args {
        if {[catch {$obj isa Test} valid] || !$valid} {
            error "bad value \"$obj\": should be Test object"
        }
    }
    set _testobjs $args

    switch -- [llength $_testobjs] {
        0 {
            # If an empty string is passed in then clear everything
            $itk_component(title) configure -text ""
            $itk_component(statusicon) configure -image ""
            $itk_component(statusmesg) configure -text ""
            $itk_component(description) configure -state normal
            $itk_component(description) delete 1.0 end
            $itk_component(description) configure -state disabled
            $itk_component(run) configure -state disabled
        }
        1 {
            set obj [lindex $_testobjs 0]
            switch [$obj getResult] {
                ? {
                    set smesg "Ready to run"
                    set sicon [Rappture::icon test64]
                    set color $itk_option(-statuspasscolor)
                }
                Pass {
                    set smesg "Test passed"
                    set sicon [Rappture::icon pass64]
                    set color $itk_option(-statuspasscolor)
                }
                Fail {
                    set smesg "Test failed"
                    set sicon [Rappture::icon fail64]
                    set color $itk_option(-statusfailcolor)
                }
                Running - Waiting {
                    set smesg "Test waiting to run"
                    set sicon [Rappture::icon wait64]
                    set color $itk_option(-statuspasscolor)
                }
                default { error "unknown test state \"[$obj getResult]\"" }
            }
            set name [lindex [split [$obj getTestInfo test.label] |] end]
            $itk_component(title) configure -text "Test: $name"
            $itk_component(statusicon) configure -image $sicon
            $itk_component(statusmesg) configure -text $smesg -foreground $color
            $itk_component(description) configure -state normal
            $itk_component(description) delete 1.0 end
            set desc [string trim [$obj getTestInfo test.description]]
            if {$desc eq ""} {
                set desc "--"
            }
            $itk_component(description) insert 1.0 $desc
            $itk_component(description) configure -state disabled
            $itk_component(run) configure -state normal
        }
        default {
            array set states { ? 0  Pass 0  Fail 0  Running 0  Waiting 0  total 0 }
            foreach obj $_testobjs {
                incr states(total)
                incr states([$obj getResult])
            }
            if {$states(total) == 1} {
                set thistest "This test"
            } else {
                set thistest "These tests"
            }

            $itk_component(title) configure \
                -text [string totitle [_plural $states(total)]]

            switch -glob -- $states(Pass)/$states(Fail)/$states(?) {
                0/0/* {
                    set smesg "Ready to run"
                    set sicon [Rappture::icon test64]
                    set color $itk_option(-statuspasscolor)
                    set desc ""
                }
                */0/0 {
                    set smesg "$thistest passed"
                    set sicon [Rappture::icon pass64]
                    set color $itk_option(-statuspasscolor)
                    set desc ""
                }
                0/*/0 {
                    set smesg "$thistest failed"
                    set sicon [Rappture::icon fail64]
                    set color $itk_option(-statusfailcolor)
                    set desc ""
                }
                0/*/* {
                    if {$states(Fail) == 1} {
                        set smesg "One of these tests failed"
                    } else {
                        set smesg "Some of these tests failed"
                    }
                    set sicon [Rappture::icon fail64]
                    set color $itk_option(-statusfailcolor)
                    set desc "[_plural $states(Fail)] failed\n[_plural $states(?)] need to run"
                }
                */*/0 {
                    if {$states(Pass) == 1} {
                        set smesg "One of these tests passed"
                    } else {
                        set smesg "Some of these tests passed"
                    }
                    set sicon [Rappture::icon test64]
                    set color $itk_option(-statuspasscolor)
                    set desc "[_plural $states(Fail)] failed\n[_plural $states(Pass)] passed"
                }
                */0/* {
                    if {$states(Pass) == 1} {
                        set smesg "One of these tests passed"
                    } else {
                        set smesg "Some of these tests passed"
                    }
                    set sicon [Rappture::icon pass64]
                    set color $itk_option(-statuspasscolor)
                    set desc "[_plural $states(Pass)] passed\n[_plural $states(?)] need to run"
                }
                default {
                    set smesg "Some tests passed, some failed"
                    set sicon [Rappture::icon fail64]
                    set color $itk_option(-statusfailcolor)
                    set desc "[_plural $states(Fail)] failed\n[_plural $states(Pass)] passed\n[_plural $states(?)] need to run"
                }
            }
            $itk_component(statusicon) configure -image $sicon
            $itk_component(statusmesg) configure -text $smesg -foreground $color
            $itk_component(description) configure -state normal
            $itk_component(description) delete 1.0 end
            $itk_component(description) insert end $desc
            $itk_component(description) configure -state disabled
            $itk_component(run) configure -state normal
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _doRun
#
# Invoked when the user presses the "Run" button to invoke the
# -runcommand script for this widget.  Invokes the command with
# the current test objects appended as arguments.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestView::_doRun {} {
    if {[string length $itk_option(-runcommand)] > 0} {
        uplevel #0 $itk_option(-runcommand) $_testobjs
    }
}

# ----------------------------------------------------------------------
# USAGE: _plural <num>
#
# Handy way of generating a plural string.  If <num> is 1, it returns
# "1 test".  Otherwise it returns "<num> tests".
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestView::_plural {num} {
    if {$num == 1} {
        return "1 test"
    } else {
        return "$num tests"
    }
}
