# ----------------------------------------------------------------------
# COMPONENT: testview - display the results of a test
#
# Entire right hand side of the regression tester.  Contains a small
# text widget, plus two TestAnalyzer widgets.  The analyzers are used to
# show the golden set of results, and the new results if the test has
# been ran.  Constructor requires a tool.xml for the tool being tested.
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

    protected method updateAnalyzer {args}
    protected method updateResult {runfile}
    protected method showStatus {text}
    protected method showDescription {text}
    public method update {datapairs}
    public method reset 

    public variable data
    protected variable _toolobj

    constructor {toolxml args} { #defined later }
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestView::constructor {toolxml args} {
   
    set _toolobj [Rappture::Tool ::#auto [Rappture::library $toolxml] \
         [file dirname $toolxml]]
 
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
    $itk_component(tabs) insert end "Result" -ipady 25 -fill both \
        -state disabled

    itk_component add analyzer {
        Rappture::Tester::TestAnalyzer $itk_component(tabs).analyzer $_toolobj
    }
    $itk_component(tabs) tab configure "Analyzer" \
        -window $itk_component(tabs).analyzer

    itk_component add result {
        text $itk_component(tabs).result
    }
    $itk_component(tabs) tab configure "Result" -window $itk_component(result)
    pack $itk_component(tabs) -expand yes -fill both -side top

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# When the -data configuration option is modified, update the display
# accordingly.  The data passed in should be a list of key value pairs
# from the TestTree widget.
# ----------------------------------------------------------------------
itcl::configbody Rappture::Tester::TestView::data {
    array set darray $data
    # Data array is empty for branch nodes.
    if {[array names darray] != ""} {
        if {$darray(ran)} {
            switch $darray(result) {
                Pass {showStatus "Test passed."}
                Fail {showStatus "Test failed."}
                Error {showStatus "Error while running test."}
            }
            if {$darray(runfile) != ""} {
                # HACK: Add a new input to differentiate between golden and
                # test result.  Otherwise, the slider at the bottom won't
                # be enabled.
                set golden [Rappture::library $darray(testxml)]
                $golden put input.run.current "Golden"
                set result [Rappture::library $darray(runfile)]
                $result put input.run.current "Test"
                updateAnalyzer $golden $result
                updateResult $data
            } else {
                updateResult
            }
        } else {
            showStatus "Test has not yet ran."
            updateResult
            if {$darray(testxml) != ""} {
                updateAnalyzer [Rappture::library $darray(testxml)]
            }
        }
        set descr [[Rappture::library $darray(testxml)] get test.description]
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
    updateResult
    showStatus ""
    showDescription ""
}

# ----------------------------------------------------------------------
# USAGE: updateAnalyzer ?<lib> <lib>...?
#
# Clears the analyzer and loads the given library objects.  Used to load 
# both the golden result as well as the test result.  Clears the
# analyzer space if no arguments are given.
# HACK: Destroys the existing analyzer widget and creates a new one.  
# Eventually this should be able to keep the same widget and swap in
# and out different result sets.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestView::updateAnalyzer {args} {
    catch {
        $itk_component(analyzer) clear
        destroy $itk_component(analyzer)
    }
    itk_component add analyzer {
        Rappture::Tester::TestAnalyzer $itk_component(tabs).analyzer $_toolobj
    }
    $itk_component(tabs) tab configure "Analyzer" \
        -window $itk_component(analyzer)
    foreach lib $args {
        $itk_component(analyzer) display $lib
    }
    
}

# ----------------------------------------------------------------------
# USAGE: updateResult ?data?
#
# Given a set of key value pairs from the test tree, update the result
# page of the testview widget.  If no arguments are given, disable the
# result page.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestView::updateResult {args} {
    if {[llength $args] == 0} {
        $itk_component(result) delete 0.0 end
        # TODO: Switch back to analyzer tab.  Why doesn't this work?
        $itk_component(tabs) invoke \
            [$itk_component(tabs) index -name "Analyzer"]
        $itk_component(tabs) tab configure "Result" -state disabled
        return
    } elseif {[llength $args] == 1} {
        array set darray [lindex $args 0]
        $itk_component(tabs) tab configure "Result" -state normal
        $itk_component(result) delete 0.0 end
        $itk_component(result) insert end "Test xml: $darray(testxml)\n"
        $itk_component(result) insert end "Runfile: $darray(runfile)\n"
        if {$darray(result) == "Fail"} {
            $itk_component(result) insert end "Diffs: $darray(diffs)\n" 
        }
    } else {
        error "wrong # args: should be \"updateResult ?data?\""
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

