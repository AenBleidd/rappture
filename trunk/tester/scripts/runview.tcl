# ----------------------------------------------------------------------
#  COMPONENT: runview - show details of a run with status/stdout
#
#  This component is used when examining a diff of type "status".
#  It shows an overview of the run, including the exit status and
#  the stdout/stderr.  This helps to visualize unexpected failures,
#  and is also useful for negative testing, where tests are expected
#  to fail but should show consistent error messages.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require RapptureGUI

namespace eval Rappture::Tester { # forward declaration }

option add *RunView.headingFont {Arial -12 bold} widgetDefault
option add *RunView.infoFont {Arial -12} widgetDefault
option add *RunView.codeFont {Courier -12} widgetDefault
option add *RunView.addedBackground #ccffcc widgetDefault
option add *RunView.deletedBackground #ffcccc widgetDefault

itcl::class Rappture::Tester::RunView {
    inherit itk::Widget 

    itk_option define -testobj testObj TestObj ""
    itk_option define -showdiffs showDiffs ShowDiffs "off"

    itk_option define -addedbackground addedBackground Background ""
    itk_option define -deletedbackground deletedBackground Background ""
    itk_option define -infofont infoFont Font ""
    itk_option define -codefont codeFont Font ""

    constructor {args} { # defined later }

    protected method _reload {}

    private variable _dispatcher ""  ;# dispatcher for !events
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::RunView::constructor {args} {
    Rappture::dispatcher _dispatcher
    $_dispatcher register !reload
    $_dispatcher dispatch $this !reload "[itcl::code $this _reload]; list"

    itk_component add overview {
        frame $itk_interior.overview
    }
    pack $itk_component(overview) -fill x

    # run failure icon on the left-hand side
    itk_component add icon {
        label $itk_component(overview).icon -image [Rappture::icon fail64]
    }
    grid $itk_component(icon) -row 0 -rowspan 6 -column 0 \
        -sticky ne -padx {0 20}

    # create label/value for "status" parameter
    itk_component add lstatus {
        label $itk_component(overview).lstatus -text "Status:"
    } {
        usual
        rename -font -headingfont headingFont Font
    }
    grid $itk_component(lstatus) -row 0 -column 1 -sticky e

    itk_component add vstatus {
        label $itk_component(overview).vstatus -justify left -anchor w
    } {
        usual
        rename -font -codefont codeFont Font
    }
    grid $itk_component(vstatus) -row 0 -column 2 -sticky w

    itk_component add lstatus2 {
        label $itk_component(overview).lstatus2 -text "Got status:"
    } {
        usual
        rename -font -headingfont headingFont Font
    }
    grid $itk_component(lstatus2) -row 0 -column 1 -sticky e

    itk_component add vstatus2 {
        label $itk_component(overview).vstatus2 -justify left -anchor w
    } {
        usual
        rename -font -codefont codeFont Font
        rename -background -deletedbackground deletedBackground Background
    }

    # set up the layout to resize properly
    grid columnconfigure $itk_component(overview) 2 -weight 1

    # create an area to show the program output -- either as text or diffs
    itk_component add body {
        frame $itk_interior.body
    }
    pack $itk_component(body) -expand yes -fill both

    # use this to show output text (-showdiffs no)
    itk_component add output {
        Rappture::Scroller $itk_component(body).scrl \
            -xscrollmode auto -yscrollmode auto
    }
    itk_component add text {
        text $itk_component(output).text -wrap none
    }
    $itk_component(output) contents $itk_component(text)

    # use this to show output diffs (-showdiffs yes)
    itk_component add diffs {
        Rappture::Tester::StringDiffs $itk_component(body).diffs \
            -title "Test output:" -title1 "Expected this:" -title2 "Got this:"
    }

    eval itk_initialize $args
}

itk::usual RunView {
    keep -background -foreground -cursor
    keep -headingfont -infofont -codefont
}

# ----------------------------------------------------------------------
# USAGE: _reload
#
# Called internally to load run information from the -testobj XML
# definition.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::RunView::_reload {} {
    set testobj $itk_option(-testobj)

    # find the test output to show, or blank
    if {$testobj ne ""} {
        if {$itk_option(-showdiffs)} {
            set st1 [$testobj getTestInfo output.status]
            set st2 [$testobj getRunInfo output.status]
            if {$st1 ne $st2} {
                # show the difference in status
                $itk_component(lstatus) configure -text "Expected status:"
                $itk_component(vstatus) configure -text $st1
                $itk_component(vstatus2) configure -text $st2

                grid $itk_component(lstatus2) -row 1 -column 1 -sticky e
                grid $itk_component(vstatus2) -row 1 -column 2 -sticky w
            } else {
                # show the status as an ordinary label
                $itk_component(lstatus) configure -text "Status:"
                $itk_component(vstatus) configure -text $st1
                grid forget $itk_component(lstatus2) $itk_component(vstatus2)
            }

            set log1 [$testobj getTestInfo output.log]
            set log2 [$testobj getRunInfo output.log]
            if {$log1 ne $log2} {
                # show the output as a diff
                $itk_component(diffs) show $log1 $log2
                pack forget $itk_component(output)
                pack $itk_component(diffs) -expand yes -fill both
            } else {
                # show the output as ordinary text
                $itk_component(text) configure -state normal
                $itk_component(text) delete 1.0 end
                $itk_component(text) insert end $log1
                $itk_component(text) configure -state disabled
                pack forget $itk_component(diffs)
                pack $itk_component(output) -expand yes -fill both
            }

            return
        }
        # not showing diffs -- show the original test
        set st1 [$testobj getTestInfo output.status]
        $itk_component(lstatus) configure -text "Status:"
        $itk_component(vstatus) configure -text $st1
        grid forget $itk_component(lstatus2) $itk_component(vstatus2)

        $itk_component(text) configure -state normal
        $itk_component(text) delete 1.0 end
        $itk_component(text) insert end [$testobj getTestInfo output.log]
        $itk_component(text) configure -state disabled
        pack forget $itk_component(diffs)
        pack $itk_component(output) -expand yes -fill both
        return
    }

    # if all else fails, blank everything out
    $itk_component(lstatus) configure -text "Status:"
    $itk_component(vstatus) configure -text ""
    grid forget $itk_component(lstatus2) $itk_component(vstatus2)
    pack forget $itk_component(output) $itk_component(diffs)
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTIONS
# ----------------------------------------------------------------------
itcl::configbody Rappture::Tester::RunView::testobj {
    $_dispatcher event -idle !reload
}
itcl::configbody Rappture::Tester::RunView::showdiffs {
    if {![string is boolean -strict $itk_option(-showdiffs)]} {
        error "bad value \"$itk_option(-showdiffs)\": should be boolean"
    }
    $_dispatcher event -idle !reload
}
