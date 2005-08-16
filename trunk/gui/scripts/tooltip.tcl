# ----------------------------------------------------------------------
#  COMPONENT: tooltip - help information that pops up beneath a widget
#
#  This file provides support for tooltips, which are little bits
#  of help information that pop up beneath a widget.
#
#  Tooltips can be registered for various widgets as follows:
#
#    Rappture::Tooltip::for .w "Some help text."
#    Rappture::Tooltip::for .x.y "Some more help text."
#
#  Tooltips can also be popped up as an error cue beneath a widget
#  with bad information:
#
#    Rappture::Tooltip::cue .w "Bad data in this widget."
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005
#  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Itk

option add *Tooltip.background white widgetDefault
option add *Tooltip.outline black widgetDefault
option add *Tooltip.borderwidth 1 widgetDefault
option add *Tooltip.font -*-helvetica-medium-r-normal-*-*-120-* widgetDefault
option add *Tooltip.wrapLength 4i widgetDefault

itcl::class Rappture::Tooltip {
    inherit itk::Toplevel

    itk_option define -outline outline Outline ""
    itk_option define -icon icon Icon ""
    itk_option define -message message Message ""

    constructor {args} { # defined below }

    public method show {where}
    public method hide {}

    public proc for {widget args}
    public proc text {widget args}
    private common catalog    ;# maps widget => message

    public proc tooltip {option args}
    private common pending "" ;# after ID for pending "tooltip show"

    public proc cue {option args}

    bind RapptureTooltip <Enter> \
        [list ::Rappture::Tooltip::tooltip pending %W]
    bind RapptureTooltip <Leave> \
        [list ::Rappture::Tooltip::tooltip cancel]
    bind RapptureTooltip <ButtonPress> \
        [list ::Rappture::Tooltip::tooltip cancel]
    bind RapptureTooltip <KeyPress> \
        [list ::Rappture::Tooltip::tooltip cancel]

    private common icons
    set dir [file dirname [info script]]
    set icons(cue) [image create photo -file [file join $dir images cue24.gif]]
}

itk::usual Tooltip {
    keep -background -outline -cursor -font
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Tooltip::constructor {args} {
    wm overrideredirect $itk_component(hull) yes
    wm withdraw $itk_component(hull)

    component hull configure -borderwidth 1 -background black
    itk_option remove hull.background hull.borderwidth

    itk_component add icon {
        label $itk_interior.icon -anchor n
    }

    itk_component add text {
        label $itk_interior.text -justify left
    } {
        usual
        keep -wraplength
    }
    pack $itk_component(text) -expand yes -fill both -ipadx 4 -ipady 4

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# USAGE: show @<x>,<y>|<widget>+/-<x>,<y>
#
# Clients use this to pop up the tooltip on the screen.  The position
# should be either a <widget> name with an optional offset +/-<x>,<y>
# (tooltip pops up beneath widget by default), or a specific root
# window coordinate of the form @x,y.
#
# If the -message has the form "@command", then the command is executed
# now, just before the tooltip is popped up, to build the message
# on-the-fly.
# ----------------------------------------------------------------------
itcl::body Rappture::Tooltip::show {where} {
    set hull $itk_component(hull)
    set signx "+"
    set signy "+"

    if {[regexp {^@([0-9]+),([0-9]+)$} $where match x y]} {
        set xpos $x
        set ypos $y
    } elseif {[regexp {^(.*)([-+])([0-9]+),([-+]?)([0-9]+)$} $where match win signx x signy y]} {
        if {$signy == ""} { set signy $signx }
        set xpos [expr {[winfo rootx $win] + $x}]
        set ypos [expr {[winfo rooty $win] + $y}]
    } elseif {[winfo exists $where]} {
        set xpos [expr {[winfo rootx $where]+10}]
        set ypos [expr {[winfo rooty $where]+[winfo height $where]}]
    } else {
        error "bad position \"$where\": should be widget+x,y, or @x,y"
    }

    if {[string index $itk_option(-message) 0] == "@"} {
        set cmd [string range $itk_option(-message) 1 end]
        if {[catch $cmd mesg] != 0} {
            bgerror $mesg
            return
        }
    } else {
        set mesg $itk_option(-message)
    }

    # if there's no message to show, forget it
    if {[string length $mesg] == 0} {
        return
    }

    # strings can't be too big, or they'll go off screen!
    if {[string length $mesg] > 1000} {
        set mesg "[string range $mesg 0 1000]..."
    }
    set pos 0
    ::for {set i 0} {$pos >= 0 && $i < 5} {incr i} {
        incr pos
        set pos [string first \n $mesg $pos]
    }
    if {$pos > 0} {
        set mesg "[string range $mesg 0 $pos]..."
    }
    $itk_component(text) configure -text $mesg

    #
    # Make sure the tooltip doesn't go off screen.  Then, put it up.
    #
    update idletasks
    if {$signx == "+"} {
        if {$xpos+[winfo reqwidth $hull] > [winfo screenwidth $hull]} {
            set xpos [expr {[winfo screenwidth $hull]-[winfo reqwidth $hull]}]
        }
        if {$xpos < 0} { set xpos 0 }
    } else {
        if {$xpos-[winfo reqwidth $hull] < 0} {
            set xpos [expr {[winfo screenwidth $hull]-[winfo reqwidth $hull]}]
        }
        set xpos [expr {[winfo screenwidth $hull]-$xpos}]
    }

    if {$signy == "+"} {
        if {$ypos+[winfo reqheight $hull] > [winfo screenheight $hull]} {
            set ypos [expr {[winfo screenheight $hull]-[winfo reqheight $hull]}]
        }
        if {$ypos < 0} { set ypos 0 }
    } else {
        if {$ypos-[winfo reqheight $hull] < 0} {
            set ypos [expr {[winfo screenheight $hull]-[winfo reqheight $hull]}]
        }
        set ypos [expr {[winfo screenheight $hull]-$ypos}]
    }

    wm geometry $hull $signx$xpos$signy$ypos
    update

    wm deiconify $hull
    raise $hull
}

# ----------------------------------------------------------------------
# USAGE: hide
#
# Takes down the tooltip, if it is showing on the screen.
# ----------------------------------------------------------------------
itcl::body Rappture::Tooltip::hide {} {
    wm withdraw $itk_component(hull)
}

# ----------------------------------------------------------------------
# USAGE: for <widget> <text>
#
# Used to register the tooltip <text> for a particular <widget>.
# This sets up bindings on the widget so that, when the mouse pointer
# lingers over the widget, the tooltip pops up automatically after
# a small delay.  When the mouse pointer leaves the widget or the
# user clicks on the widget, it cancels the tip.
#
# If the <text> has the form "@command", then the command is executed
# just before the tip pops up to build the message on-the-fly.
# ----------------------------------------------------------------------
itcl::body Rappture::Tooltip::for {widget text} {
    set catalog($widget) $text

    set btags [bindtags $widget]
    set i [lsearch $btags RapptureTooltip]
    if {$i < 0} {
        set i [lsearch $btags [winfo class $widget]]
        if {$i < 0} {set i 0}
        set btags [linsert $btags $i RapptureTooltip]
        bindtags $widget $btags
    }
}

# ----------------------------------------------------------------------
# USAGE: text <widget> ?<text>?
#
# Used to query or set the text used for the tooltip for a widget.
# This is done automatically when you call the "for" proc, but it
# is sometimes handy to query or change the text later.
# ----------------------------------------------------------------------
itcl::body Rappture::Tooltip::text {widget args} {
    if {[llength $args] == 0} {
        if {[info exists catalog($widget)]} {
            return $catalog($widget)
        }
        return ""
    } elseif {[llength $args] == 1} {
        set str [lindex $args 0]
        set catalog($widget) $str
    } else {
        error "wrong # args: should be \"text widget ?str?\""
    }
}

# ----------------------------------------------------------------------
# USAGE: tooltip pending <widget> ?@<x>,<y>|+<x>,<y>?
# USAGE: tooltip show <widget> ?@<x>,<y>|+<x>,<y>?
# USAGE: tooltip cancel
#
# This is invoked automatically whenever the user clicks somewhere
# inside or outside of the editor.  If the <X>,<Y> coordinate is
# outside the editor, then we assume the user is done and wants to
# take the editor down.  Otherwise, we do nothing, and let the entry
# bindings take over.
# ----------------------------------------------------------------------
itcl::body Rappture::Tooltip::tooltip {option args} {
    switch -- $option {
        pending {
            if {[llength $args] < 1 || [llength $args] > 2} {
                error "wrong # args: should be \"tooltip pending widget ?@x,y?\""
            }
            set widget [lindex $args 0]
            set loc [lindex $args 1]

            if {![info exists catalog($widget)]} {
                error "can't find tooltip for $widget"
            }
            if {$pending != ""} {
                after cancel $pending
            }
            set pending [after 1500 [itcl::code tooltip show $widget $loc]]
        }
        show {
            if {[llength $args] < 1 || [llength $args] > 2} {
                error "wrong # args: should be \"tooltip pending widget ?@x,y?\""
            }
            set widget [lindex $args 0]
            set loc [lindex $args 1]

            if {[winfo exists $widget]} {
                .rappturetooltip configure -message $catalog($widget)
                if {[string index $loc 0] == "@"} {
                    .rappturetooltip show $loc
                } elseif {[regexp {^[-+]} $loc]} {
                    .rappturetooltip show $widget$loc
                } else {
                    .rappturetooltip show $widget
                }
            }
        }
        cancel {
            if {$pending != ""} {
                after cancel $pending
                set pending ""
            }
            .rappturetooltip hide
        }
        default {
            error "bad option \"$option\": should be show, pending, cancel"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: cue <location> <message>
# USAGE: cue hide
#
# Clients use this to show a <message> in a tooltip cue at the
# specified <location>, which can be a widget name or a root coordinate
# at @x,y.
# ----------------------------------------------------------------------
itcl::body Rappture::Tooltip::cue {option args} {
    if {"hide" == $option} {
        grab release .rappturetoolcue
        .rappturetoolcue hide
    } elseif {[regexp {^@[0-9]+,[0-9]+$} $option] || [winfo exists $option]} {
        if {[llength $args] != 1} {
            error "wrong # args: should be \"cue location message\""
        }
        set loc $option
        set mesg [lindex $args 0]

        .rappturetoolcue configure -message $mesg
        .rappturetoolcue show $loc

        #
        # Add a binding to all widgets so that any keypress will
        # take this cue down.
        #
        set cmd [bind all <KeyPress>]
        if {![regexp {Rappture::Tooltip::cue} $cmd]} {
            bind all <KeyPress> "+[list ::Rappture::Tooltip::cue hide]"
            bind all <KeyPress-Return> "+ "
        }

        #
        # If nobody has the pointer, then grab it.  Otherwise,
        # we assume the pop-up editor or someone like that has
        # the grab, so we don't need to impose a grab here.
        #
        if {"" == [grab current]} {
            update
            while {[catch {grab set -global .rappturetoolcue}]} {
                after 100
            }
        }
    } else {
        error "bad option \"$option\": should be hide, a widget name, or @x,y"
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -icon
# ----------------------------------------------------------------------
itcl::configbody Rappture::Tooltip::icon {
    if {"" == $itk_option(-icon)} {
        $itk_component(icon) configure -image ""
        pack forget $itk_component(icon)
    } else {
        $itk_component(icon) configure -image $itk_option(-icon)
        pack $itk_component(icon) -before $itk_component(text) \
            -side left -fill y
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -outline
# ----------------------------------------------------------------------
itcl::configbody Rappture::Tooltip::outline {
    component hull configure -background $itk_option(-outline)
}

# create a tooltip widget to show tool tips
Rappture::Tooltip .rappturetooltip

# any click on any widget takes down the tooltip
bind all <Leave> [list ::Rappture::Tooltip::tooltip cancel]
bind all <ButtonPress> [list ::Rappture::Tooltip::tooltip cancel]

# create a tooltip widget to show error cues
Rappture::Tooltip .rappturetoolcue \
    -icon $Rappture::Tooltip::icons(cue) \
    -background black -outline #333333 -foreground white

# when cue is up, it has a grab, and any click brings it down
bind .rappturetoolcue <ButtonPress> [list ::Rappture::Tooltip::cue hide]
