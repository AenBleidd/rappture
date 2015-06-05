# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: scroller - adds scrollbars to any standard Tk widget
#
#  This widget provides automatic scrollbars for any standard Tk
#  widget.  The scrolled widget should be created as a child of this
#  widget, and is connected by calling the "contents" method.  Calling
#  contents with the keyword "frame" creates an internal frame that
#  allows any collection of widgets to be scrolled.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

option add *Scroller.xScrollMode off widgetDefault
option add *Scroller.yScrollMode auto widgetDefault
option add *Scroller.xScrollSide bottom widgetDefault
option add *Scroller.yScrollSide right widgetDefault
option add *Scroller.width 0 widgetDefault
option add *Scroller.height 0 widgetDefault

itcl::class Rappture::Scroller {
    inherit itk::Widget

    itk_option define -xscrollmode xScrollMode XScrollMode ""
    itk_option define -yscrollmode yScrollMode YScrollMode ""
    itk_option define -xscrollside xScrollSide XScrollSide ""
    itk_option define -yscrollside yScrollSide YScrollSide ""
    itk_option define -width width Width 0
    itk_option define -height height Height 0

    constructor {args} { # defined below }
    destructor { # defined below }

    public method contents {{frame "!@#query"}}

    protected method _widget2sbar {which args}
    protected method _fixsbar {which {state ""}}
    protected method _fixframe {which}
    protected method _fixsize {}
    protected method _lock {option which}

    private variable _dispatcher "" ;# dispatcher for !events
    private variable _contents ""   ;# widget being controlled
    private variable _frame ""      ;# for "contents frame" calls
    private variable _lock          ;# for _lock on x/y scrollbar
}

itk::usual Scroller {
    keep -background -activebackground -activerelief
    keep -cursor
    keep -highlightcolor -highlightthickness
    keep -troughcolor
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Scroller::constructor {args} {
    array set _lock { x 0 y 0 }

    Rappture::dispatcher _dispatcher

    $_dispatcher register !fixframe-inner
    $_dispatcher dispatch $this !fixframe-inner \
        "[itcl::code $this _fixframe inner]; list"

    $_dispatcher register !fixframe-outer
    $_dispatcher dispatch $this !fixframe-outer \
        "[itcl::code $this _fixframe outer]; list"

    $_dispatcher register !fixsize
    $_dispatcher dispatch $this !fixsize \
        "[itcl::code $this _fixsize]; list"

    itk_component add xsbar {
        scrollbar $itk_interior.xsbar -orient horizontal
    }
    itk_component add ysbar {
        scrollbar $itk_interior.ysbar -orient vertical
    }

    # we don't fix scrollbars when window is withdrawn, so
    # fix them whenever a window pops up
    bind $itk_component(hull) <Map> "
        [itcl::code $this _fixsbar x]
        [itcl::code $this _fixsbar y]
    "

    grid rowconfigure $itk_component(hull) 1 -weight 1
    grid columnconfigure $itk_component(hull) 1 -weight 1

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Scroller::destructor {} {
    after cancel [itcl::code $this _lock reset x]
    after cancel [itcl::code $this _lock reset y]
}

# ----------------------------------------------------------------------
# USAGE: contents ?<widget>|frame?
#
# Used to get/set the widget that is being scrolled.  With no args,
# it returns the name of the widget currently connected to the
# scrollbars.  Otherwise, the argument specifies a widget to be
# controlled by the scrollbar.  If the argument is the keyword
# "frame", then this method creates its own internal frame, which
# can be packed with other widgets, and returns its name.
# ----------------------------------------------------------------------
itcl::body Rappture::Scroller::contents {{widget "!@#query"}} {
    if {$widget == "!@#query"} {
        if {$_contents == $_frame} {
            return $_frame.f
        }
        return $_contents
    }

    #
    # If the widget is "", then unhook any existing widget.
    #
    if {$widget == ""} {
        if {$_contents != ""} {
            $_contents configure -xscrollcommand "" -yscrollcommand ""
            grid forget $_contents
        }
        $itk_component(xsbar) configure -command ""
        $itk_component(ysbar) configure -command ""
        set _contents ""

        return ""
    }

    #
    # For the "frame" keyword, create a canvas that can be scrolled
    # and return it as the frame being scrolled.
    #
    if {$widget == "frame"} {
        if {$_frame == ""} {
            set _frame [canvas $itk_component(hull).ifr -highlightthickness 0]
            frame $_frame.f
            $_frame create window 0 0 -anchor nw -window $_frame.f -tags frame
            bind $_frame.f <Map> \
                [itcl::code $_dispatcher event -idle !fixframe-inner]
            bind $_frame.f <Configure> \
                [itcl::code $_dispatcher event -idle !fixframe-inner]
            bind $_frame <Configure> \
                [itcl::code $_dispatcher event -idle !fixframe-outer]
        }
        set widget $_frame
    }

    #
    # Plug the specified widget into the scrollbars for this widget.
    #
    contents ""
    grid $widget -row 1 -column 1 -sticky nsew
    $widget configure \
        -xscrollcommand [itcl::code $this _widget2sbar x] \
        -yscrollcommand [itcl::code $this _widget2sbar y]

    $itk_component(xsbar) configure -command [list $widget xview]
    $itk_component(ysbar) configure -command [list $widget yview]
    set _contents $widget

    if {[string equal "x11" [tk windowingsystem]]} {
        bind ${widget} <4> { %W yview scroll -5 units }
        bind ${widget} <5> { %W yview scroll 5 units }
    } else {
        bind ${widget} <MouseWheel> {
            %W yview scroll [expr {- (%D / 120) * 4}] units
        }
    }
    if {$widget == $_frame} {
        return $_frame.f
    }
    return $widget
}

# ----------------------------------------------------------------------
# USAGE: _widget2sbar <which> ?args...?
#
# Used internally to handle communication from the widget to the
# scrollbar.  If the scrollbars are in "auto" mode, this provides a
# hook where we can fix their display.
# ----------------------------------------------------------------------
itcl::body Rappture::Scroller::_widget2sbar {which args} {
    if {$itk_option(-xscrollmode) == "auto"} {
        _fixsbar x
    }
    if {$itk_option(-yscrollmode) == "auto"} {
        _fixsbar y
    }
    eval $itk_component(${which}sbar) set $args
}

# ----------------------------------------------------------------------
# USAGE: _fixsbar <which> ?<state>?
#
# Used internally to show/hide the scrollbar in the <which> direction,
# which is either "x" or "y".  If the scrollbar is "on", then it is
# always displayed.  If "off", never displayed.  And if "auto", then
# it is displayed if needed for the current view.
# ----------------------------------------------------------------------
itcl::body Rappture::Scroller::_fixsbar {which {state ""}} {
    if {![winfo ismapped $itk_component(hull)]} {
        #
        # If we're not on yet screen, bail out!  This keeps bad
        # numbers (from an empty or partially constructed widget)
        # from prematurely influencing the scrollbar.
        #
        return
    }

    if {$state == ""} {
        switch -- [string tolower $itk_option(-${which}scrollmode)] {
            on - 1 - true - yes  { set state 1 }
            off - 0 - false - no { set state 0 }
            auto {
                set state 0
                if {$_contents != ""} {
                    set lims [$_contents ${which}view]
                    if {[lindex $lims 0] != 0 || [lindex $lims 1] != 1} {
                        set state 1
                    }
                }
            }
            default {
                set state 0
            }
        }
    }

    set row 0
    set col 0
    switch -- [string tolower $itk_option(-${which}scrollside)] {
        top {
            set row 0
            set col 1
        }
        bottom {
            set row 2
            set col 1
        }
        left {
            set row 1
            set col 0
        }
        right {
            set row 1
            set col 2
        }
        default {
            set row 0
            set col 0
        }
    }

    # show/hide the scrollbar depending on the desired state
    switch -- $which {
        x {
            if {$state} {
                if {$col == 1} {
                    grid $itk_component(xsbar) -row $row -column $col -sticky ew
                }
            } else {
                # handle the lock on the "forget" side, so scrollbar
                # tends to appear, rather than disappear
                if {![_lock active x]} {
                    grid forget $itk_component(xsbar)
                    _lock set x
                }
            }
        }
        y {
            if {$state} {
                if {$row == 1} {
                    grid $itk_component(ysbar) -row $row -column $col -sticky ns
                }
            } else {
                # handle the lock on the "forget" side, so scrollbar
                # tends to appear, rather than disappear
                if {![_lock active y]} {
                    grid forget $itk_component(ysbar)
                    _lock set y
                }
            }
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _fixframe <which>
#
# Invoked automatically whenever the canvas representing the "frame"
# keyword is resized.  Updates the scrolling limits for the canvas
# to the new size.
# ----------------------------------------------------------------------
itcl::body Rappture::Scroller::_fixframe {which} {
    switch -- $which {
        inner {
            set w [winfo reqwidth $_frame.f]
            set h [winfo reqheight $_frame.f]
            $_frame configure -scrollregion [list 0 0 $w $h]
            _fixframe outer
            _lock reset x
            _lock reset y
            $_dispatcher event -idle !fixsize
        }
        outer {
            if {[winfo width $_frame] > [winfo reqwidth $_frame.f]} {
                $_frame itemconfigure frame -width [winfo width $_frame]
            } else {
                $_frame itemconfigure frame -width 0
            }
            if {[winfo height $_frame] > [winfo reqheight $_frame.f]} {
                $_frame itemconfigure frame -height [winfo height $_frame]
            } else {
                $_frame itemconfigure frame -height 0
            }
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _fixsize
#
# Used internally to update the size options for the widget
# whenever the -width/-height options change.
# ----------------------------------------------------------------------
itcl::body Rappture::Scroller::_fixsize {} {
    if {$itk_option(-width) == "0" && $itk_option(-height) == "0"} {
        # for default size, let the frame being controlled set the size
        grid propagate $itk_component(hull) yes
        if {$_frame == "$itk_component(hull).ifr"} {
            set w [expr {[winfo reqwidth $_frame.f]+2}]
            set h [winfo reqheight $_frame.f]
            $_frame configure -width $w -height $h
        }
    } else {
        # for specific size, set the overall size of the widget
        grid propagate $itk_component(hull) no
        set w $itk_option(-width); if {$w == "0"} { set w 1i }
        set h $itk_option(-height); if {$h == "0"} { set h 1i }
        component hull configure -width $w -height $h
    }
}

# ----------------------------------------------------------------------
# USAGE: _lock set <which>
# USAGE: _lock reset <which>
# USAGE: _lock active <which>
#
# Used internally to lock out vibrations when the x-scrollbar pops
# into view.  When the x-scrollbar pops up, it reduces the space
# available for the widget.  For some widgets (e.g., text widget)
# this changes the view.  A long line may fall off screen, and the
# x-scrollbar will no longer be necessary.  If the x-scrollbar just
# appeared, then its lock is active, signalling that it should stay
# up.
# ----------------------------------------------------------------------
itcl::body Rappture::Scroller::_lock {option which} {
    switch -- $option {
        set {
            set _lock($which) 1
            after cancel [itcl::code $this _lock reset $which]
            after 50 [itcl::code $this _lock reset $which]
        }
        reset {
            set _lock($which) 0
        }
        active {
            return $_lock($which)
        }
        default {
            error "bad option \"$option\": should be set, reset, active"
        }
    }
}

# ----------------------------------------------------------------------
# OPTION: -xscrollmode
# ----------------------------------------------------------------------
itcl::configbody Rappture::Scroller::xscrollmode {
    _fixsbar x
}

# ----------------------------------------------------------------------
# OPTION: -yscrollmode
# ----------------------------------------------------------------------
itcl::configbody Rappture::Scroller::yscrollmode {
    _fixsbar y
}

# ----------------------------------------------------------------------
# OPTION: -xscrollside
# ----------------------------------------------------------------------
itcl::configbody Rappture::Scroller::xscrollside {
    _fixsbar x
}

# ----------------------------------------------------------------------
# OPTION: -yscrollside
# ----------------------------------------------------------------------
itcl::configbody Rappture::Scroller::yscrollside {
    _fixsbar y
}

# ----------------------------------------------------------------------
# OPTION: -width
# ----------------------------------------------------------------------
itcl::configbody Rappture::Scroller::width {
    # check for proper value
    winfo pixels $itk_component(hull) $itk_option(-width)

    $_dispatcher event -idle !fixsize
}

# ----------------------------------------------------------------------
# OPTION: -height
# ----------------------------------------------------------------------
itcl::configbody Rappture::Scroller::height {
    # check for proper value
    winfo pixels $itk_component(hull) $itk_option(-height)

    $_dispatcher event -idle !fixsize
}
