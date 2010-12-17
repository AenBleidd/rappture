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

    protected method clear {}
    protected method showTest {testxml}
    protected method showResult {runfile}
    protected method showStatus {text}
    protected method showDescription {text}
    protected method changeTabs {}
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
            -selectbackground $itk_option(-background) \
            -selectcommand [itcl::code $this changeTabs]
    } {
    }
    $itk_component(tabs) insert end "Analyzer"
    $itk_component(tabs) insert end "Result" -state disabled
    pack $itk_component(tabs) -expand no -fill y -side left

    itk_component add nb {
        Rappture::Notebook $itk_interior.nb
    }

    $itk_component(nb) insert end "Analyzer" "Result"
    itk_component add analyzer {
        Rappture::Tester::TestAnalyzer \
            [$itk_component(nb) page "Analyzer"].analyzer $_toolobj
    } 
    pack $itk_component(analyzer) -expand yes -fill both -side top
    $itk_component(nb) current "Analyzer"

    itk_component add result {
        text [$itk_component(nb) page "Result"].result
    }
    pack $itk_component(result) -expand yes -fill both -side top

    pack $itk_component(nb) -expand yes -fill both -side left
    eval itk_initialize $args
}

itk::usual TestView {
    keep -background -foreground -font
}

# ----------------------------------------------------------------------
# USAGE: changeTabs
#
# Used internally to switch notebook pages when a tab is selected. Note
# that the tab names must match the notebook page names.
# TODO: Is there a better way of connecting the tabs to the notebook?
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestView::changeTabs {} {
    set cur [$itk_component(tabs) get [$itk_component(tabs) index select]]
    $itk_component(nb) current $cur
}

# ----------------------------------------------------------------------
# USAGE: clear
#
# Clears both result viewers by deleting the testanalyzer widgets.
# Eventually, this should clear the results from the analyzer without
# needing to destroy it.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestView::clear {} {
    catch {destroy $itk_component(analyzer)}
    $itk_component(nb) current "Analyzer"
    $itk_component(tabs) invoke [$itk_component(tabs) index -name "Result"]
    $itk_component(tabs) tab configure "Result" -state disabled 
    showStatus ""
    showDescription ""
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
    itk_component add analyzer {
        Rappture::Tester::TestAnalyzer \
            [$itk_component(nb) page "Analyzer"].analyzer $_toolobj
    }
    pack $itk_component(analyzer) -expand yes -fill both -side top
    $itk_component(analyzer) display $testxml
}

# ----------------------------------------------------------------------
# TODO: fill this in
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestView::showResult {runfile} {
    $itk_component(tabs) tab configure "Result" -state normal 
    $itk_component(result) delete 0.0 end
    $itk_component(result) insert end $runfile
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
                Pass {showStatus "Test passed."}
                Fail {showStatus "Test failed."}
                Error {showStatus "Error while running test."}
            }
            if {$data(runfile) != ""} {
                # Display new results.
                showResult $data(runfile)
            }
        } else {
            showStatus "Test has not yet ran."
        }
        set descr [[Rappture::library $data(testxml)] get test.description]
        if {$descr == ""} {
            set descr "No description."
        }
        showDescription $descr 
    } 
}

