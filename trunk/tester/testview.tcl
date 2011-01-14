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

    itk_option define -test test Test ""

    constructor {args} { #defined later }

    protected method reset 
    protected method showDescription {text}
    protected method showStatus {text}
    protected method updateResults {args}
    protected method updateInfo {args}
    protected method updateInputs {args}

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
    $itk_component(tabs) insert end "Results" -ipady 25 -fill both
    $itk_component(tabs) insert end "Info" -ipady 25 -fill both -state disabled
    $itk_component(tabs) insert end "Inputs" -ipady 25 -fill both \
        -state disabled

    itk_component add results {
        Rappture::ResultsPage $itk_component(tabs).results
    }
    $itk_component(tabs) tab configure "Results" \
        -window $itk_component(tabs).results

    itk_component add info {
        text $itk_component(tabs).info
    }
    $itk_component(tabs) tab configure "Info" -window $itk_component(info)
    pack $itk_component(tabs) -expand yes -fill both -side top

    itk_component add inputs {
        blt::treeview $itk_component(tabs).inputs -separator . -autocreate true
    } {
        keep -foreground -font -cursor
    }
    $itk_component(tabs) tab configure "Inputs" -window $itk_component(inputs)

    eval itk_initialize $args

}

# ----------------------------------------------------------------------
# When the -test configuration option is modified, update the display
# accordingly.  The data passed in should be a Test object, or the
# empty string to clear the display.
# ----------------------------------------------------------------------
itcl::configbody Rappture::Tester::TestView::test {
    set test $itk_option(-test)
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
        } else {
            showStatus "Test has not yet ran."
        }
        updateResults
        updateInfo
        updateInputs
        set descr [[$test getTestobj] get test.description]
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
    updateResults
    updateInfo
    updateInputs
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
# USAGE: updateResults
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestView::updateResults {} {
    $itk_component(results) clear -nodelete
    set test $itk_option(-test)
    if {$test == ""} {
        # Already cleared, do nothing.
        # TODO: Eventually display some kinds of message here.
    } else {
        set test $itk_option(-test)
        $itk_component(results) load [$test getTestobj]
        if {[$test hasRan]} {
            $itk_component(results) load [$test getRunobj]
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: updateInfo
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestView::updateInfo {} {
    set test $itk_option(-test)
    if {$test == "" || ![$test hasRan]} {
        $itk_component(info) delete 0.0 end
        set index [$itk_component(tabs) index -name "Results"]
        $itk_component(tabs) select $index
        $itk_component(tabs) focus $index
        $itk_component(tabs) tab configure "Info" -state disabled
    } else {
        set test $itk_option(-test) 
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
    }
}

itcl::body Rappture::Tester::TestView::updateInputs {} {
    set test $itk_option(-test)
    if {$test == ""} {
        set index [$itk_component(tabs) index -name "Results"]
        $itk_component(tabs) select $index
        $itk_component(tabs) focus $index
        $itk_component(tabs) tab configure "Inputs" -state disabled
    } else {
        set test $itk_option(-test)
        $itk_component(tabs) tab configure "Inputs" -state normal
    }
}

