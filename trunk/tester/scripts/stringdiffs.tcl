# ----------------------------------------------------------------------
#  COMPONENT: stringdiffs - show diff info between two strings
#
#  This component is used when examining the details of a particular
#  diff.  It shows two strings with either inline or side-by-side
#  diffs.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2010-2011  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require RapptureGUI

namespace eval Rappture::Tester { # forward declaration }

option add *StringDiffs.titleFont {Arial -12 bold} widgetDefault
option add *StringDiffs.bodyFont {Courier -12} widgetDefault
option add *StringDiffs.bodyBackground white widgetDefault

itcl::class Rappture::Tester::StringDiffs {
    inherit itk::Widget 

    itk_option define -title title Title ""
    itk_option define -title1 title1 Title1 ""
    itk_option define -title2 title2 Title2 ""

    constructor {args} { # defined later }
    method show {v1 v2}

    protected method _yview {args}
    protected method _ysbar {args}
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::StringDiffs::constructor {args} {
    # viewer for inline diffs
    itk_component add inline {
        frame $itk_interior.inline
    }

    itk_component add title {
        label $itk_component(inline).title -justify left -anchor w
    } {
        usual
        rename -font -titlefont titleFont Font
    }
    pack $itk_component(title) -side top -fill x

    itk_component add scrl {
        Rappture::Scroller $itk_component(inline).scrl \
            -xscrollmode auto -yscrollmode auto
    }
    pack $itk_component(scrl) -expand yes -fill both

    itk_component add body {
        Rappture::Diffview $itk_component(scrl).diffs \
            -highlightthickness 0 \
            -diff 1->2 -layout inline
    } {
        keep -foreground -cursor
        keep -addedbackground -addedforeground
        keep -deletedbackground -deletedforeground -overstrike
        keep -changedbackground -changedforeground
        rename -background -bodybackground bodyBackground Background
        rename -font -bodyfont bodyFont Font
    }
    $itk_component(scrl) contents $itk_component(body)

    # viewer for side-by-side diffs
    itk_component add sidebyside {
        frame $itk_interior.sbys
    }

    itk_component add title1 {
        label $itk_component(sidebyside).title1 -justify left -anchor w
    } {
        usual
        rename -font -titlefont titleFont Font
    }
    itk_component add body1 {
        Rappture::Diffview $itk_component(sidebyside).s1 \
            -highlightthickness 0 \
            -diff 2->1 -layout sidebyside
    } {
        keep -background -foreground -cursor
        keep -addedbackground -addedforeground
        keep -deletedbackground -deletedforeground -overstrike
        keep -changedbackground -changedforeground
        rename -background -bodybackground bodyBackground Background
        rename -font -bodyfont bodyFont Font
    }
    itk_component add xsbar1 {
        scrollbar $itk_component(sidebyside).xsbar1 -orient horizontal \
            -command [list $itk_component(body1) xview]
    }

    itk_component add title2 {
        label $itk_component(sidebyside).title2 -justify left -anchor w
    } {
        usual
        rename -font -titlefont titleFont Font
    }
    itk_component add body2 {
        Rappture::Diffview $itk_component(sidebyside).s2 \
            -highlightthickness 0 \
            -diff 1->2 -layout sidebyside
    } {
        keep -background -foreground -cursor
        keep -addedbackground -addedforeground
        keep -deletedbackground -deletedforeground -overstrike
        keep -changedbackground -changedforeground
        rename -background -bodybackground bodyBackground Background
        rename -font -bodyfont bodyFont Font
    }
    itk_component add xsbar2 {
        scrollbar $itk_component(sidebyside).xsbar2 -orient horizontal \
            -command [list $itk_component(body2) xview]
    }

    itk_component add ysbar {
        scrollbar $itk_component(sidebyside).ysbar -orient vertical \
            -command [itcl::code $this _yview]
    }
    $itk_component(body1) configure \
        -xscrollcommand [list $itk_component(xsbar1) set] \
        -yscrollcommand [itcl::code $this _ysbar body1]
    $itk_component(body2) configure \
        -xscrollcommand [list $itk_component(xsbar2) set] \
        -yscrollcommand [itcl::code $this _ysbar body2]

    grid $itk_component(title1) -row 0 -column 0 -sticky nsew
    grid $itk_component(body1) -row 1 -column 0 -sticky nsew
    grid $itk_component(xsbar1) -row 2 -column 0 -sticky ew
    grid $itk_component(ysbar) -row 1 -column 1 -sticky ns
    grid $itk_component(title2) -row 0 -column 2 -sticky nsew
    grid $itk_component(body2) -row 1 -column 2 -sticky nsew
    grid $itk_component(xsbar2) -row 2 -column 2 -sticky ew
    grid columnconfigure $itk_component(sidebyside) 0 -weight 1
    grid columnconfigure $itk_component(sidebyside) 2 -weight 1
    grid rowconfigure $itk_component(sidebyside) 1 -weight 1

    eval itk_initialize $args
}

itk::usual StringDiffs {
    keep -background -foreground -cursor
    keep -titlefont -bodyfont
}

# ----------------------------------------------------------------------
# USAGE: show <v1> <v2>
#
# Loads two values into the viewer and shows their differences.
# If the strings are short, the diffs are shown inline.  Otherwise,
# they are show with side-by-side viewers.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::StringDiffs::show {v1 v2} {
    #
    # HACK ALERT!  The Diffview widget doesn't handle tabs at all.
    #   We'll convert all tabs to a few spaces until we get a chance
    #   to fix it properly.
    #
    set v1 [string map [list \t "   "] $v1]
    set v2 [string map [list \t "   "] $v2]

    #
    # Figure out whether to show inline diffs or side-by-side.
    # If the strings are short, it looks better inline.  Otherwise,
    # use side-by-side.
    #
    foreach w [pack slaves $itk_interior] {
        pack forget $w
    }
    if {[llength [split $v1 \n]] > 10 || [llength [split $v2 \n]] > 10} {
        set which "sidebyside"
    } else {
        set which "inline"
    }
    pack $itk_component($which) -expand yes -fill both

    switch -- $which {
        inline {
            $itk_component(body) text 1 $v1
            $itk_component(body) text 2 $v2
            $itk_component(body1) text 1 ""
            $itk_component(body1) text 2 ""
            $itk_component(body2) text 1 ""
            $itk_component(body2) text 2 ""
        }
        sidebyside {
            $itk_component(body1) text 1 $v1
            $itk_component(body1) text 2 $v2
            $itk_component(body2) text 1 $v1
            $itk_component(body2) text 2 $v2
            $itk_component(body) text 1 ""
            $itk_component(body) text 2 ""
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _yview <arg> <arg>...
#
# Called whenever the scrollbar changes the y-view of the diffs.
# Sends the new command along to both views so they are aligned.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::StringDiffs::_yview {args} {
    eval $itk_component(body1) yview $args
    eval $itk_component(body2) yview $args
}

# ----------------------------------------------------------------------
# USAGE: _ysbar <whichChanged> <arg> <arg>...
#
# Called whenever the y-view of one widget changes.  Copies the 
# current view from the <whichChanged> widget to the other side,
# and updates the bubble to display the correct view.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::StringDiffs::_ysbar {which args} {
    switch -- $which {
        body1 {
            set pos [lindex [$itk_component(body1) yview] 0]
            $itk_component(body2) yview moveto $pos
        }
        body2 {
            set pos [lindex [$itk_component(body2) yview] 0]
            $itk_component(body1) yview moveto $pos
        }
    }
    eval $itk_component(ysbar) set $args
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTIONS: -title -title1 -title2
# ----------------------------------------------------------------------
itcl::configbody Rappture::Tester::StringDiffs::title {
    # if there is no -title for inline diffs, use -title1
    if {$itk_option(-title) eq ""} {
        $itk_component(title) configure -text $itk_option(-title1)
    } else {
        $itk_component(title) configure -text $itk_option(-title)
    }
}

itcl::configbody Rappture::Tester::StringDiffs::title1 {
    # if there is no -title for inline diffs, use -title1
    if {$itk_option(-title) eq ""} {
        $itk_component(title) configure -text $itk_option(-title1)
    }
    $itk_component(title1) configure -text $itk_option(-title1)
}

itcl::configbody Rappture::Tester::StringDiffs::title2 {
    $itk_component(title2) configure -text $itk_option(-title2)
}
