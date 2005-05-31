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
#  Copyright (c) 2004-2005
#  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Itk

option add *Scroller.xScrollMode off widgetDefault
option add *Scroller.yScrollMode auto widgetDefault
option add *Scroller.width 0 widgetDefault
option add *Scroller.height 0 widgetDefault

itcl::class Rappture::Scroller {
    inherit itk::Widget

    itk_option define -xscrollmode xScrollMode XScrollMode ""
    itk_option define -yscrollmode yScrollMode YScrollMode ""
    itk_option define -width width Width 0
    itk_option define -height height Height 0

    constructor {args} { # defined below }

    public method contents {{frame "!@#query"}}

    protected method _widget2sbar {which args}
    protected method _fixsbar {which {state ""}}
    protected method _fixframe {which}
    protected method _lock {option}

    private variable _contents ""   ;# widget being controlled
    private variable _frame ""      ;# for "contents frame" calls
    private variable _lock 0        ;# for _lock on x-scrollbar
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
    itk_component add xsbar {
        scrollbar $itk_interior.xsbar -orient horizontal
    }
    itk_component add ysbar {
        scrollbar $itk_interior.ysbar -orient vertical
    }

    grid rowconfigure $itk_component(hull) 0 -weight 1
    grid columnconfigure $itk_component(hull) 0 -weight 1

    eval itk_initialize $args
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
            bind $_frame.f <Configure> [itcl::code $this _fixframe inner]
            bind $_frame <Configure> [itcl::code $this _fixframe outer]
        }
        set widget $_frame
    }

    #
    # Plug the specified widget into the scrollbars for this widget.
    #
    contents ""
    grid $widget -row 0 -column 0 -sticky nsew
    $widget configure \
        -xscrollcommand [itcl::code $this _widget2sbar x] \
        -yscrollcommand [itcl::code $this _widget2sbar y]

    $itk_component(xsbar) configure -command [list $widget xview]
    $itk_component(ysbar) configure -command [list $widget yview]
    set _contents $widget

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

    # show/hide the scrollbar depending on the desired state
    switch -- $which {
        x {
            if {$state} {
                grid $itk_component(xsbar) -row 1 -column 0 -sticky ew
                _lock set
            } else {
                if {![_lock active]} {
                    grid forget $itk_component(xsbar)
                }
            }
        }
        y {
            if {$state} {
                grid $itk_component(ysbar) -row 0 -column 1 -sticky ns
            } else {
                grid forget $itk_component(ysbar)
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
            $_frame configure -scrollregion [$_frame bbox all]
        }
        outer {
            $_frame itemconfigure frame -width [winfo width $_frame]
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _lock set
# USAGE: _lock reset
# USAGE: _lock active
#
# Used internally to lock out vibrations when the x-scrollbar pops
# into view.  When the x-scrollbar pops up, it reduces the space
# available for the widget.  For some widgets (e.g., text widget)
# this changes the view.  A long line may fall off screen, and the
# x-scrollbar will no longer be necessary.  If the x-scrollbar just
# appeared, then its lock is active, signalling that it should stay
# up.
# ----------------------------------------------------------------------
itcl::body Rappture::Scroller::_lock {option} {
    switch -- $option {
        set {
            set _lock 1
            after cancel [itcl::code $this _lock reset]
            after 50 [itcl::code $this _lock reset]
        }
        reset {
            set _lock 0
        }
        active {
            return $_lock
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
# OPTION: -width
# ----------------------------------------------------------------------
itcl::configbody Rappture::Scroller::width {
    if {$itk_option(-width) == "0"} {
        if {$itk_option(-height) == "0"} {
            grid propagate $itk_component(hull) yes
        } else {
            component hull configure -width 1i
        }
    } else {
        grid propagate $itk_component(hull) no
        component hull configure -width $itk_option(-width)
    }
}

# ----------------------------------------------------------------------
# OPTION: -height
# ----------------------------------------------------------------------
itcl::configbody Rappture::Scroller::height {
    if {$itk_option(-height) == "0"} {
        if {$itk_option(-width) == "0"} {
            grid propagate $itk_component(hull) yes
        } else {
            component hull configure -height 1i
        }
    } else {
        grid propagate $itk_component(hull) no
        component hull configure -height $itk_option(-height)
    }
}
