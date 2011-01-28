# ----------------------------------------------------------------------
#  COMPONENT: testview - display the results of a test
#
#  Entire right hand side of the regression tester.  Displays the
#  golden test results, and compares them to the new results if the test
#  has been ran.  Also show tree representation of all inputs and
#  outputs.  The -test configuration option is used to provide a Test
#  object to display.
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
    protected method updateResults {}
    protected method updateInputs {}
    protected method updateOutputs {}

}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestView::constructor {args} {

    itk_component add status {
        label $itk_interior.status
    }
    pack $itk_component(status) -expand no -fill none -side top -anchor w 

    itk_component add descScroller {
        Rappture::Scroller $itk_interior.descScroller \
            -xscrollmode auto -yscrollmode auto
    }

    itk_component add description {
        text $itk_interior.descScroller.description -height 0 -wrap word \
            -relief flat
    }
    $itk_component(descScroller) contents $itk_component(description)
    pack $itk_component(descScroller) -expand no -fill x -side top

    itk_component add tabs {
        blt::tabset $itk_interior.tabs -borderwidth 0 -relief flat \
            -side left -tearoff 0 -highlightthickness 0 \
            -selectbackground $itk_option(-background)
    } {
    }
    $itk_component(tabs) insert end "Results" -ipady 25 -fill both
    $itk_component(tabs) insert end "Inputs" -ipady 25 -fill both \
        -state disabled
    $itk_component(tabs) insert end "Outputs" -ipady 25 -fill both \
        -state disabled

    itk_component add results {
        Rappture::ResultsPage $itk_component(tabs).results
    }
    $itk_component(tabs) tab configure "Results" \
        -window $itk_component(tabs).results

    itk_component add inputScroller {
        Rappture::Scroller $itk_component(tabs).inputScroller \
            -xscrollmode auto -yscrollmode auto
    }

    itk_component add inputs {
        blt::treeview $itk_component(inputScroller).inputs -separator . \
            -autocreate true
    } {
        keep -foreground -font -cursor
    }
    $itk_component(inputs) column insert end "Value"
    $itk_component(inputScroller) contents $itk_component(inputs)
    $itk_component(tabs) tab configure "Inputs" \
        -window $itk_component(inputScroller)

    itk_component add outputScroller {
        Rappture::Scroller $itk_component(tabs).outputScroller \
            -xscrollmode auto -yscrollmode auto
    }

    itk_component add outputs {
        blt::treeview $itk_component(outputScroller).outputs -separator . \
            -autocreate true
    } {
        keep -foreground -font -cursor
    }
    $itk_component(outputs) column insert end "Status"
    $itk_component(outputScroller) contents $itk_component(outputs)
    $itk_component(tabs) tab configure "Outputs" \
        -window $itk_component(outputScroller)

    pack $itk_component(tabs) -expand yes -fill both -side top

    eval itk_initialize $args

}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -test
#
# When the -test configuration option is modified, update the display
# accordingly.  The data passed in should be a Test object, or an empty 
# string to clear the display.
# ----------------------------------------------------------------------
itcl::configbody Rappture::Tester::TestView::test {
    set test $itk_option(-test)
    # If an empty string is passed in then clear everything
    if {$test == ""} {
        reset
    } else {
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
            showStatus "Test has not yet run."
        }
        updateResults
        updateInputs
        updateOutputs
        set descr [[$test getTestobj] get test.description]
        if {$descr == ""} {
            set descr "No description."
        }
        showDescription $descr
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
    updateInputs
    updateOutputs
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

# Used internally to update the results tab according to the test
# currently specified by the -test configuration option.  Show the
# golden results contained in the test xml, and if the the test has been
# ran, show the new results as well.
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
        if {[$test hasRan]  && [Rappture::library isvalid [$test getRunobj]]} {
            $itk_component(results) load [$test getRunobj]
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: updateInputs
#
# Used internally to update the inputs tab according to the test
# currently specified by the -test configuration option.  Shows a tree
# representation of all inputs given in the test xml and their given
# values.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestView::updateInputs {} {
    $itk_component(inputs) delete 0
    set test $itk_option(-test)
    if {$test != ""} {
        $itk_component(tabs) tab configure "Inputs" -state normal
        foreach pair [$test getInputs] {
            set path [lindex $pair 0]
            set val [lindex $pair 1]
            $itk_component(inputs) insert end $path -data [list Value $val]
        }
        $itk_component(inputs) open -recurse root
    }
}

# ----------------------------------------------------------------------
# USAGE: updateOutputs
#
# Used internally to update the outputs tab according to the test
# currently specified by the -test configuration option.  Shows a tree
# representation of all outputs in the runfile generated by the last run
# of the test, along with their status (ok, diff, added, or missing).
# Disable the outputs tab if test has not been ran, or resulted in an
# error.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestView::updateOutputs {} {
    $itk_component(outputs) delete 0
    set test $itk_option(-test)
    if {$test != "" && [$test hasRan] && [$test getResult] != "Error"} {
        $itk_component(tabs) tab configure "Outputs" -state normal
        foreach pair [$test getOutputs] {
            set path [lindex $pair 0]
            set status [lindex $pair 1]
            $itk_component(outputs) insert end $path -data [list Status $status]
        }
        $itk_component(outputs) open -recurse root
    } else {
        $itk_component(tabs) tab configure "Outputs" -state disabled
        # Switch back to results tab if the outputs tab is open and the
        # selected test has not been ran.
        if {[$itk_component(tabs) index select] == \
            [$itk_component(tabs) index -name "Outputs"]} {
            set index [$itk_component(tabs) index -name "Results"]
            $itk_component(tabs) select $index
            $itk_component(tabs) focus $index
        }
    } 
}
        
        

