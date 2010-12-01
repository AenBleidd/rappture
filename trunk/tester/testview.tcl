# ----------------------------------------------------------------------
# COMPONENT: testview - display the results of a test
#
# Doesn't do anything yet.  TODO: Fill this in later.
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
    public method display {runfile testxml}
    public method showDefault {}
    public method showText {text}

    constructor {tool args} { #defined later }
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestView::constructor {tool args} {
    
    itk_component add txt {
        text $itk_interior.txt -height 1
    }
    pack $itk_component(txt) -expand yes -fill x

    itk_component add analyzer1 {
        Rappture::Tester::TestAnalyzer $itk_interior.analyzer1 $tool
    } 
    pack $itk_component(analyzer1) -expand yes -fill both

    itk_component add analyzer2 {
        Rappture::Tester::TestAnalyzer $itk_interior.analyzer2 $tool
    }
    pack $itk_component(analyzer2) -expand yes -fill both

    eval itk_initialize $args

    showDefault
}

itk::usual TestView {
    keep -background -foreground -font
}

itcl::body Rappture::Tester::TestView::clear {} {
    $itk_component(analyzer1) clear
    $itk_component(analyzer2) clear
}

itcl::body Rappture::Tester::TestView::display {runfile testxml} {
    $itk_component(analyzer1) display $runfile
    $itk_component(analyzer2) display $testxml
}

itcl::body Rappture::Tester::TestView::showDefault {} {
    $itk_component(txt) configure -state normal 
    $itk_component(txt) delete 0.0 end
    $itk_component(txt) insert end "Default"
    $itk_component(txt) configure -state disabled
    clear
}

itcl::body Rappture::Tester::TestView::showText {text} {
    $itk_component(txt) configure -state normal
    $itk_component(txt) delete 0.0 end
    $itk_component(txt) insert end "$text"
    $itk_component(txt) configure -state disabled
}
