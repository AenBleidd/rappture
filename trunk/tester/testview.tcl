# ----------------------------------------------------------------------
#  COMPONENT: testview - display the results of a test
#
#  Entire right hand side of the regression tester.  Displays the
#  golden test results, and compares them to the new results if the test
#  has been ran.  The -test configuration option is used to provide a
#  Test object to display.
# ======================================================================
#  AUTHOR:  Ben Rafferty, Purdue University
#  Copyright (c) 2010  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require BLT 

namespace eval Rappture::Tester::TestView { #forward declaration }

option add *TestView.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault
option add *TestView.codeFont \
    -*-courier-medium-r-normal-*-12-* widgetDefault
option add *TestView.textFont \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault
option add *TestView.boldTextFont \
    -*-helvetica-bold-r-normal-*-12-* widgetDefault

itcl::class Rappture::Tester::TestView {
    inherit itk::Widget 

    public variable test 

    constructor {args} { #defined later }

    protected method reset 
    protected method showDescription {text}
    protected method showStatus {text}
    protected method updateAnalyzer {args}
    protected method updateInfo {runfile}

}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestView::constructor {args} {

    itk_component add status {
        label $itk_interior.status
    }
    pack $itk_component(status) -expand no -fill none -side top -anchor w 

    itk_component add scroller {
        Rappture::Scroller $itk_interior.scroller \
            -xscrollmode auto -yscrollmode auto
    }

    itk_component add description {
        text $itk_interior.scroller.description -height 0 -wrap word \
            -relief flat
    }
    $itk_component(scroller) contents $itk_component(description)
    pack $itk_component(scroller) -expand no -fill x -side top

    itk_component add tabs {
        blt::tabset $itk_interior.tabs -borderwidth 0 -relief flat \
            -side left -tearoff 0 -highlightthickness 0 \
            -selectbackground $itk_option(-background)
    } {
    }
    $itk_component(tabs) insert end "Analyzer" -ipady 25 -fill both
    $itk_component(tabs) insert end "Info" -ipady 25 -fill both \
        -state disabled

    #itk_component add analyzer {
    #    Rappture::ResultsPage $itk_component(tabs).analyzer
    #}
    #$itk_component(tabs) tab configure "Analyzer" \
    #    -window $itk_component(tabs).analyzer

    itk_component add info {
        text $itk_component(tabs).info
    }
    $itk_component(tabs) tab configure "Info" -window $itk_component(info)
    pack $itk_component(tabs) -expand yes -fill both -side top

    eval itk_initialize $args

}

# ----------------------------------------------------------------------
# When the -test configuration option is modified, update the display
# accordingly.  The data passed in should be a Test object, or the
# empty string to clear the display.
# ----------------------------------------------------------------------
itcl::configbody Rappture::Tester::TestView::test {
    # Data array is empty for branch nodes.
    if {$test != ""} {
        if {![$test isa Test]} {
            error "-test option must be a Test object.  $test was given."
        }
        if {[$test hasRan]} {
            switch [$test getResult] {
                Pass {showStatus "Test passed."}
                Fail {showStatus "Test failed."}
                Error {showStatus "Error while running test."}
            }
            if {[$test getRunfile] != ""} {
                # HACK: Add a new input to differentiate between golden and
                # test result.  Otherwise, the slider at the bottom won't
                # be enabled.
                set golden [Rappture::library [$test getTestxml]]
                $golden put input.run.current "Golden"
                set runfile [Rappture::library [$test getRunfile]]
                $runfile put input.run.current "Test result"
                updateAnalyzer $golden $runfile
                updateInfo $test
            } else {
                updateInfo
            }
        } else {
            showStatus "Test has not yet ran."
            updateInfo
            if {[$test getTestxml] != ""} {
                updateAnalyzer [Rappture::library [$test getTestxml]]
            }
        }
        set descr [[Rappture::library [$test getTestxml]] get test.description]
        if {$descr == ""} {
            set descr "No description."
        }
        showDescription $descr
    } else {
       # Clear everything if branch node selected
       reset 
    }
}

itk::usual TestView {
    keep -background -foreground -font
}

# ----------------------------------------------------------------------
# USAGE: reset 
#
# Resets the entire TestView widget back to the default state.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestView::reset {} {
    updateAnalyzer
    updateInfo
    showStatus ""
    showDescription ""
}

# ----------------------------------------------------------------------
# USAGE: showDescription <text>
#
# Displays a string in the description text space near the top of the
# widget.  If given an empty string, disable the sunken relief effect
# to partially hide the text box.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestView::showDescription {text} {
    $itk_component(description) configure -state normal
    $itk_component(description) delete 0.0 end
    $itk_component(description) insert end "$text"
    $itk_component(description) configure -state disabled
    if {$text == ""} {
        $itk_component(description) configure -relief flat
    } else {
        $itk_component(description) configure -relief sunken
    }
}

# ----------------------------------------------------------------------
# USAGE: showStatus <text>
#
# Displays a string in the status info space at the top of the widget.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestView::showStatus {text} {
    $itk_component(status) configure -text "$text"
}

# ----------------------------------------------------------------------
# USAGE: updateAnalyzer ?<lib> <lib>...?
#
# Clears the analyzer and loads the given library objects.  Used to load 
# both the golden result as well as the test result.  Clears the
# analyzer space if no arguments are given.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestView::updateAnalyzer {args} {
    # HACK: Strange errors happen when the same widget is cleared and
    # loaded with new data.  Instead, destroy the results page and
    # create a new one.
    catch {
        $itk_component(analyzer) clear
        destroy $itk_component(analyzer)
    }
    itk_component add analyzer {
        Rappture::ResultsPage $itk_component(tabs).analyzer
    }
    $itk_component(tabs) tab configure "Analyzer" \
        -window $itk_component(analyzer)
    foreach lib $args {
        $itk_component(analyzer) load $lib
    }
}

# ----------------------------------------------------------------------
# USAGE: updateInfo ?test?
#
# Given a set of key value pairs from the test tree, update the info 
# page of the testview widget.  If no arguments are given, disable the
# info page.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestView::updateInfo {args} {
    if {[llength $args] == 0} {
        $itk_component(info) delete 0.0 end
        # TODO: Switch back to analyzer tab.  Why doesn't this work?
        $itk_component(tabs) invoke \
            [$itk_component(tabs) index -name "Analyzer"]
        $itk_component(tabs) tab configure "Info" -state disabled
        return
    } elseif {[llength $args] == 1} {
        set testxml [$test getTestxml]
        set runfile [$test getRunfile]
        $itk_component(tabs) tab configure "Info" -state normal
        $itk_component(info) delete 0.0 end
        $itk_component(info) insert end "Test xml: $testxml\n"
        $itk_component(info) insert end "Runfile: $runfile\n"
        if {[$test getResult] == "Fail"} {
            set diffs [$test getDiffs]
            set missing [$test getMissing]
            set added [$test getAdded]
            $itk_component(info) insert end "Diffs: $diffs\n" 
            $itk_component(info) insert end "Missing: $missing\n"
            $itk_component(info) insert end "Added: $added\n"
        }
    } else {
        error "wrong # args: should be \"updateInfo ?data?\""
    }
}

