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

    public method clear {}
    public method showTest {testxml}
    public method showResult {runfile}
    public method showText {text}
    public method update {datapairs}

    private variable _toolobj

    constructor {toolxml args} { #defined later }
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestView::constructor {toolxml args} {
   
    set _toolobj [Rappture::Tool ::#auto [Rappture::library $toolxml] \
         [file dirname $toolxml]]
 
    itk_component add txt {
        text $itk_interior.txt -height 1
    }
    pack $itk_component(txt) -expand no -fill x -side top

    itk_component add analyzer1 {
        Rappture::Tester::TestAnalyzer $itk_interior.analyzer1 $_toolobj
    } 
    pack $itk_component(analyzer1) -expand no -fill both -side top

    itk_component add analyzer2 {
        Rappture::Tester::TestAnalyzer $itk_interior.analyzer2 $_toolobj
    }
    pack $itk_component(analyzer2) -expand no -fill both -side top

    eval itk_initialize $args
}

itk::usual TestView {
    keep -background -foreground -font
}

# ----------------------------------------------------------------------
# USAGE: clear
#
# Clears both result viewers by deleting the testanalyzer widgets.
# Eventually, this should clear the results from the analyzer without
# needing to destroy it.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestView::clear {} {
    #$itk_component(analyzer1) clear
    catch {destroy $itk_component(analyzer1)}
    catch {destroy $itk_component(analyzer2)}
    $itk_component(txt) delete 0.0 end
}

# ----------------------------------------------------------------------
# USAGE: showTest <testxml>
#
# Displays a new set of golden results by deleting the existing
# analyzer and creating a new one.  Eventually, this should be able to
# swap the currently visible set of resuls without needing to destroy
# the widget.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestView::showTest {testxml} {
    itk_component add analyzer1 {
        Rappture::Tester::TestAnalyzer $itk_interior.analyzer1 $_toolobj
    }
    pack $itk_component(analyzer1) -expand no -fill both -side top
    $itk_component(analyzer1) display $testxml
}

# ----------------------------------------------------------------------
# USAGE: showResult <runfile>
#
# Displays a new test result by deleting the existing analyzer and
# creating a new one.  Eventually, this should be able to swap the
# currently visible set of results without needing to destroy the
# widget.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestView::showResult {runfile} {
    itk_component add analyzer2 {
        Rappture::Tester::TestAnalyzer $itk_interior.analyzer2 $_toolobj
    }
    pack $itk_component(analyzer2) -expand no -fill both -side top
    $itk_component(analyzer2) display $runfile
}

# ----------------------------------------------------------------------
# USAGE: showText <text>
#
# Displays a string in the text space at the top of the widget.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestView::showText {text} {
    $itk_component(txt) configure -state normal
    $itk_component(txt) delete 0.0 end
    $itk_component(txt) insert end "$text"
    $itk_component(txt) configure -state disabled
}

# ----------------------------------------------------------------------
# USAGE: update <datapairs>
#
# Given a list of key value pairs from the test tree, shows the
# golden result, plus the new result if the test has been ran.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestView::update {datapairs} {
    clear
    array set data $datapairs
    # Data array is empty for branch nodes.
    if {[array names data] != ""} {
        if {$data(testxml) != ""} {
            # Display golden results.
            showTest $data(testxml)
        }
        if {$data(ran)} {
            switch $data(result) {
                Pass {showText "Test passed."}
                Fail {showText "Diffs: $data(diffs)"}
                Error {showText "Error while running test."}
            }
            if {$data(runfile) != ""} {
                # Display new results.
                showResult $data(runfile)
            }
        } else {
            showText "Test has not yet ran."
        }
    } 
}

