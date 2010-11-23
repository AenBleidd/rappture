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

    constructor {} { #defined later }
}

itcl::body Rappture::Regression::TestView::constructor {} {
    puts "Constructing TestView."
    itk_component add txt {
        text $itk_interior.txt
    }
    pack $itk_component(txt) -expand yes -fill both
    $itk_component(txt) insert end "TestView text area..."
    $itk_component(txt) configure -state disabled
}

