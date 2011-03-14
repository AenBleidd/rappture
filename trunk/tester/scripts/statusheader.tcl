# ----------------------------------------------------------------------
#  COMPONENT: statusheader - header for detailed view of diffs
#
#  This component appears across the top of the screen when you choose
#  to examine a particular difference within a test.  It is similar
#  to a single entry in StatusList, but also gives buttons to navigate
#  back and forth within in the list.
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

option add *StatusHeader.font {Arial -12} widgetDefault
option add *StatusHeader.titleFont {Arial -12 bold} widgetDefault
option add *StatusHeader.subTitleFont {Arial -10} widgetDefault
option add *StatusHeader.troughBackground #666666 widgetDefault
option add *StatusHeader.troughForeground #d9d9d9 widgetDefault
option add *StatusHeader.bubbleBackground #d9d9d9 widgetDefault
option add *StatusHeader.bubbleForeground black widgetDefault
option add *StatusHeader*Button.borderWidth 1 widgetDefault

itcl::class Rappture::Tester::StatusHeader {
    inherit itk::Widget 

    itk_option define -font font Font ""
    itk_option define -titlefont titleFont Font ""
    itk_option define -subtitlefont subTitleFont Font ""

    itk_option define -bubblebackground bubbleBackground BubbleBackground ""
    itk_option define -bubbleforeground bubbleForeground BubbleForeground ""

    itk_option define -nextcommand nextCommand NextCommand ""
    itk_option define -prevcommand prevCommand PrevCommand ""
    itk_option define -closecommand closeCommand CloseCommand ""

    constructor {args} { # defined later }
    destructor { # defined later }

    public method show {args}

    protected method _redraw {}
    protected method _fixSize {}
    protected method _nav {which action}

    private variable _dispatcher ""  ;# dispatcher for !events
    private variable _entry ""       ;# StatusEntry object holding options
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::StatusHeader::constructor {args} {
    Rappture::dispatcher _dispatcher
    $_dispatcher register !redraw
    $_dispatcher dispatch $this !redraw "[itcl::code $this _redraw]; list"
    $_dispatcher register !size
    $_dispatcher dispatch $this !size "[itcl::code $this _fixSize]; list"

    set _entry [Rappture::Tester::StatusEntry #auto]

    pack propagate $itk_interior off

    itk_component add btmline {
        frame $itk_interior.btm -height 2 -borderwidth 0 -relief flat
    } {
        usual
        rename -background -bubbleforeground bubbleForeground BubbleForeground
    }
    pack $itk_component(btmline) -side bottom -fill x

    itk_component add header {
        canvas $itk_interior.hd -relief flat -highlightthickness 0
    } {
        usual
        ignore -highlightthickness
        rename -background -troughbackground troughBackground Background
    }
    pack $itk_component(header) -expand yes -fill both

    bind $itk_component(header) <Configure> \
        [list $_dispatcher event -idle !redraw]

    # close button
    itk_component add close {
        button $itk_component(header).close \
            -relief flat -overrelief raised \
            -bitmap [Rappture::icon dismiss] \
            -command [itcl::code $this _nav close invoke]
    } {
        usual
        ignore -relief -overrelief
        rename -background -troughbackground troughBackground Background
        rename -foreground -troughforeground troughForeground Foreground
        rename -highlightbackground -troughbackground troughBackground Background
    }

    eval itk_initialize $args

    $_dispatcher event -idle !size
}

itk::usual StatusHeader {
    keep -background -foreground -cursor
    keep -bubblebackground -bubbleforeground
    keep -font -titlefont -subtitlefont
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::StatusHeader::destructor {} {
    itcl::delete object $_entry
}

# ----------------------------------------------------------------------
# USAGE: show ?-option value -option value ...?
#
# Changes the information being displayed within this header.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::StatusHeader::show {args} {
    eval $_entry configure $args
    $_dispatcher event -idle !redraw
}


# ----------------------------------------------------------------------
# USAGE: _redraw
#
# Used internally to update the detailed list of items maintained
# by this widget.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::StatusHeader::_redraw {} {
    set c $itk_component(header)
    $c delete all

    set w [winfo width $c]
    set h [winfo height $c]

    # put the close button up in the corner
    if {[string length $itk_option(-closecommand)] > 0} {
      $c create window [expr {$w-6}] 6 -anchor ne -window $itk_component(close)
    }

    set wbtn [image width [Rappture::icon blue-fwd]]
    set pad [expr {[winfo reqwidth $itk_component(close)] + $wbtn/2 + 12}]

    # draw the arc on the left side and the prev button
    $c create arc $pad -$h [expr {$pad+3*$h}] [expr {2*$h}] \
        -start 90 -extent 180 -outline $itk_option(-bubbleforeground) \
        -fill $itk_option(-bubblebackground)

    if {[string length $itk_option(-prevcommand)] > 0} {
        set state "normal"
    } else {
        set state "disabled"
    }
    set id [$c create image $pad [expr {$h/2}] -anchor c -state $state \
        -image [Rappture::icon blue-rev] \
        -disabledimage [Rappture::icon blue-rev-dis] -tags prev]
    $c bind $id <ButtonPress> [itcl::code $this _nav prev press]
    $c bind $id <ButtonRelease> [itcl::code $this _nav prev release]

    # draw the arc on the right side and the next button
    $c create arc [expr {$w-$pad-3*$h}] -$h [expr {$w-$pad}] [expr {2*$h}] \
        -start 270 -extent 180 -outline $itk_option(-bubbleforeground) \
        -fill $itk_option(-bubblebackground)

    if {[string length $itk_option(-nextcommand)] > 0} {
        set state "normal"
    } else {
        set state "disabled"
    }
    set id [$c create image [expr {$w-$pad}] [expr {$h/2}] -anchor c \
        -image [Rappture::icon blue-fwd] \
        -disabledimage [Rappture::icon blue-fwd-dis] -tags next]
    $c bind $id <ButtonPress> [itcl::code $this _nav next press]
    $c bind $id <ButtonRelease> [itcl::code $this _nav next release]

    # block in the middle
    $c create rectangle [expr {$pad+$h/2}] 0 [expr {$w-$pad-$h/2}] $h \
        -outline "" -fill $itk_option(-bubblebackground)

    # text within the block
    set wbtn [image width [Rappture::icon blue-rev]]
    set x0 [expr {$pad + $wbtn/2 + 20}]
    set y0 4
    set tlineh [font metrics $itk_option(-titlefont) -linespace]
    set stlineh [font metrics $itk_option(-subtitlefont) -linespace]
    set fg $itk_option(-bubbleforeground)

    set icon [$_entry cget -icon]
    set x1 $x0
    if {$icon ne ""} {
        set iw [image width $icon]
        $c create image [expr {$x0+$iw}] $y0 -anchor ne -image $icon
        set x1 [expr {$x0+$iw+6}]
    }
    set y1 $y0

    set title [$_entry cget -title]
    if {$title ne ""} {
        $c create text [expr {$x1-4}] $y1 -anchor nw -text $title \
            -font $itk_option(-titlefont) -fill $fg
        set y1 [expr {$y1+$tlineh+2}]
    }

    set subtitle [$_entry cget -subtitle]
    if {$subtitle ne ""} {
        $c create text $x1 $y1 -anchor nw -text $subtitle \
            -font $itk_option(-subtitlefont) -fill $fg
        set y1 [expr {$y1+$stlineh+2}]
    }

    set body [$_entry cget -body]
    if {$body ne ""} {
        # a little space between the title/subtitle and the body
        if {$y1 != $y0} { incr y1 4 }

        set id [$c create text $x1 $y1 -anchor nw -text $body \
            -font $itk_option(-font) -fill $fg]

        foreach {tx0 ty0 tx1 ty1} [$c bbox $id] break
        set y1 [expr {$y1 + ($ty1-$ty0)}]
    }
}

# ----------------------------------------------------------------------
# USAGE: _fixSize
#
# Used internally to compute the requested side for this widget.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::StatusHeader::_fixSize {} {
    set c $itk_component(header)
    set wbtn [image width [Rappture::icon blue-rev]]
    set wtitle [font measure $itk_option(-titlefont) [string repeat X 40]]
    set wmin [expr {3*$wbtn + 3*$wbtn + $wtitle}]

    set htitle [font metrics $itk_option(-titlefont) -linespace]
    set hstitle [font metrics $itk_option(-subtitlefont) -linespace]
    set hbtitle [font metrics $itk_option(-font) -linespace]
    set hmin [expr {4 + $htitle + $hstitle + 8 + $hbtitle + 4}]

    component hull configure -width $wmin -height $hmin
}

# ----------------------------------------------------------------------
# USAGE: _nav next|prev|close press|release|invoke
#
# Called internally when the user clicks on the "<<" or ">>" buttons
# on either side of the header.  Handles the button up/down, and
# invokes the appropriate command script to handle the action.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::StatusHeader::_nav {which action} {
    set c $itk_component(header)
    switch -- $action {
        press {
            array set icons [list \
                prev [Rappture::icon blue-rev-dep] \
                next [Rappture::icon blue-fwd-dep] \
            ]
            if {[$c itemcget $which -state] != "disabled"} {
                $c itemconfigure $which -image $icons($which)
            }
        }
        release {
            array set icons [list \
                prev [Rappture::icon blue-rev] \
                next [Rappture::icon blue-fwd] \
            ]
            if {[$c itemcget $which -state] != "disabled"} {
                $c itemconfigure $which -image $icons($which)
            }
            _nav $which invoke
        }
        invoke {
            set opt -${which}command
            if {[string length $itk_option($opt)] > 0} {
                uplevel #0 $itk_option($opt)
            }
        }
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTIONS
# ----------------------------------------------------------------------
itcl::configbody Rappture::Tester::StatusHeader::font {
    $_dispatcher event -idle !redraw
}
itcl::configbody Rappture::Tester::StatusHeader::titlefont {
    $_dispatcher event -idle !redraw
}
itcl::configbody Rappture::Tester::StatusHeader::subtitlefont {
    $_dispatcher event -idle !redraw
}
itcl::configbody Rappture::Tester::StatusHeader::bubblebackground {
    $_dispatcher event -idle !redraw
}
itcl::configbody Rappture::Tester::StatusHeader::bubbleforeground {
    $_dispatcher event -idle !redraw
}

itcl::configbody Rappture::Tester::StatusHeader::nextcommand {
    if {[string length $itk_option(-nextcommand)] > 0} {
        $itk_component(header) itemconfigure next -state normal
    } else {
        $itk_component(header) itemconfigure next -state disabled
    }
}
itcl::configbody Rappture::Tester::StatusHeader::prevcommand {
    if {[string length $itk_option(-prevcommand)] > 0} {
        $itk_component(header) itemconfigure prev -state normal
    } else {
        $itk_component(header) itemconfigure prev -state disabled
    }
}
itcl::configbody Rappture::Tester::StatusHeader::closecommand {
    $_dispatcher event -idle !redraw
}
