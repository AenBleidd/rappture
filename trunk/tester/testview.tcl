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

namespace eval Rappture::Regression::TestView { #forward declaration }

itcl::class Rappture::Regression::TestView {
    inherit itk::Widget

    public method showDefault {}
    public method showText {text}

    constructor {args} { #defined later }
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Regression::TestView::constructor {args} {
    itk_component add txt {
        text $itk_interior.txt
    }
    pack $itk_component(txt) -expand yes -fill both
    showDefault

    eval itk_initialize $args
}

itk::usual TestView {
    keep -background -foreground -font
}

itcl::body Rappture::Regression::TestView::showDefault {} {
    $itk_component(txt) configure -state normal 
    $itk_component(txt) delete 0.0 end
    $itk_component(txt) insert end "Default"
    $itk_component(txt) configure -state disabled
}

itcl::body Rappture::Regression::TestView::showText {text} {
    $itk_component(txt) configure -state normal
    $itk_component(txt) delete 0.0 end
    $itk_component(txt) insert end "$text"
    $itk_component(txt) configure -state disabled
}
