# ----------------------------------------------------------------------
#  COMPONENT: switch - on/off switch
#
#  This widget is used to control a (boolean) on/off value.  It shows
#  a little light with the state of the switch, and offers a combobox
#  to control the values.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005
#  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Itk
package require BLT

option add *Switch.width 30 widgetDefault
option add *Switch.height 20 widgetDefault
option add *Switch.onColor red widgetDefault
option add *Switch.offColor gray widgetDefault
option add *Switch.valuePosition "right" widgetDefault
option add *Switch.textBackground #cccccc widgetDefault

itcl::class Rappture::Switch {
    inherit itk::Widget

    itk_option define -valueposition valuePosition ValuePosition ""
    itk_option define -oncolor onColor Color ""
    itk_option define -offcolor offColor Color ""
    itk_option define -onimage onImage Image ""
    itk_option define -offimage offImage Image ""
    itk_option define -width width Width 0
    itk_option define -height height Height 0

    constructor {args} { # defined below }

    public method value {args}

    protected method _redraw {}
    protected method _fixState {}
    protected method _resize {}
    protected method _hilite {comp state}
    protected method _presets {option}

    private variable _value "no"  ;# value for this widget

    blt::bitmap define SwitchArrow {
        #define arrow_width 9
        #define arrow_height 4
        static unsigned char arrow_bits[] = {
           0x7f, 0x00, 0x3e, 0x00, 0x1c, 0x00, 0x08, 0x00};
    }
}
                                                                                
itk::usual Switch {
    keep -cursor -font -foreground -background
    keep -selectbackground -selectforeground -selectborderwidth
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Switch::constructor {args} {
    itk_component add icon {
        canvas $itk_interior.icon -borderwidth 0 -highlightthickness 0
    } {
        usual
        ignore -highlightthickness -highlightbackground -highlightcolor
    }
    pack $itk_component(icon) -side left
    bind $itk_component(icon) <Configure> [itcl::code $this _redraw]

    itk_component add -protected vframe {
        frame $itk_interior.vframe
    }

    itk_component add value {
        label $itk_component(vframe).value -borderwidth 1 -width 7 \
            -textvariable [itcl::scope _value]
    } {
        rename -background -textbackground textBackground Background
    }
    pack $itk_component(value) -side left -expand yes -fill both

    bind $itk_component(value) <Enter> [itcl::code $this _hilite value on]
    bind $itk_component(value) <Leave> [itcl::code $this _hilite value off]

    itk_component add presets {
        button $itk_component(vframe).psbtn -bitmap SwitchArrow \
            -borderwidth 1 -highlightthickness 0 -relief flat
    } {
        usual
        ignore -borderwidth -relief -highlightthickness
        rename -background -textbackground textBackground Background
    }

    bind $itk_component(presets) <Enter> [itcl::code $this _hilite presets on]
    bind $itk_component(presets) <Leave> [itcl::code $this _hilite presets off]

    itk_component add presetlist {
        Rappture::Dropdownlist $itk_component(presets).plist \
            -postcommand [itcl::code $this _presets post] \
            -unpostcommand [itcl::code $this _presets unpost] \
    }
    $itk_component(presetlist) insert end 1 yes 0 no

    bind $itk_component(presetlist) <<DropdownlistSelect>> \
        [itcl::code $this _presets select]

    bind $itk_component(value) <ButtonPress> \
        [list $itk_component(presetlist) post $itk_component(vframe) left]
    $itk_component(presets) configure -command \
        [list $itk_component(presetlist) post $itk_component(vframe) left]

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# USAGE: value ?-check? ?<newval>?
#
# Clients use this to query/set the value for this widget.  With
# no args, it returns the current value for the widget.  If the
# <newval> is specified, it sets the value of the widget and
# sends a <<Value>> event.  If the -check flag is included, the
# new value is not actually applied, but just checked for correctness.
# ----------------------------------------------------------------------
itcl::body Rappture::Switch::value {args} {
    set onlycheck 0
    set i [lsearch -exact $args -check]
    if {$i >= 0} {
        set onlycheck 1
        set args [lreplace $args $i $i]
    }

    if {[llength $args] == 1} {
        set newval [lindex $args 0]
        if {![string is boolean -strict $newval]} {
            error "Should be a boolean value"
        }
        set newval [expr {($newval) ? "yes" : "no"}]

        if {$onlycheck} {
            return
        }
        set _value $newval
        _fixState
        event generate $itk_component(hull) <<Value>>

    } elseif {[llength $args] != 0} {
        error "wrong # args: should be \"value ?-check? ?newval?\""
    }
    return $_value
}

# ----------------------------------------------------------------------
# USAGE: _redraw
#
# Used internally to redraw the gauge on the internal canvas based
# on the current value and the size of the widget.  In this simple
# base class, the gauge is drawn as a colored block, with an optional
# image in the middle of it.
# ----------------------------------------------------------------------
itcl::body Rappture::Switch::_redraw {} {
    set c $itk_component(icon)
    set w [winfo width $c]
    set h [winfo height $c]
    set s [expr {($w < $h) ? $w : $h}]
    set r [expr {0.5*$s-3}]
    set wm [expr {0.5*$w}]
    set hm [expr {0.5*$h}]

    $c delete all

    if {$itk_option(-onimage) == "" && $itk_option(-offimage) == ""} {
        $c create oval [expr {$wm-$r+2}] [expr {$hm-$r+2}] \
            [expr {$wm+$r+1}] [expr {$hm+$r+1}] -fill gray -outline ""
        $c create oval [expr {$wm-$r}] [expr {$hm-$r}] \
            [expr {$wm+$r+1}] [expr {$hm+$r+1}] -fill gray -outline ""
        $c create oval [expr {$wm-$r}] [expr {$hm-$r}] \
            [expr {$wm+$r}] [expr {$hm+$r}] -fill "" -outline black \
            -tags main

        $c create arc [expr {$wm-$r+2}] [expr {$hm-$r+2}] \
            [expr {$wm+$r-2}] [expr {$hm+$r-2}] -fill "" -outline "" \
            -start 90 -extent -60 -style arc -tags hilite

        $c create arc [expr {$wm-$r+2}] [expr {$hm-$r+2}] \
            [expr {$wm+$r-2}] [expr {$hm+$r-2}] -fill "" -outline "" \
            -start -90 -extent -60 -style arc -tags lolite
    } else {
        $c create image [expr {0.5*$w}] [expr {0.5*$h}] \
            -anchor center -image "" -tags bimage
    }
    _fixState
}

# ----------------------------------------------------------------------
# USAGE: _fixState
#
# Used internally to fix the colors associated with the on/off
# control.  This has an effect only if there is no -onimage or
# -offimage.
# ----------------------------------------------------------------------
itcl::body Rappture::Switch::_fixState {} {
    set c $itk_component(icon)

    if {$_value} {
        if {$itk_option(-onimage) != "" || $itk_option(-offimage) != ""} {
            $c itemconfigure bimage -image $itk_option(-onimage)
        } else {
            set color $itk_option(-oncolor)
            $c itemconfigure main -fill $color
            $c itemconfigure hilite -outline \
                [Rappture::color::brightness $color 1.0]
            $c itemconfigure lolite -outline \
                [Rappture::color::brightness $color -0.3]
        }
    } else {
        if {$itk_option(-onimage) != "" || $itk_option(-offimage) != ""} {
            $c itemconfigure bimage -image $itk_option(-offimage)
        } else {
            set color $itk_option(-offcolor)
            $c itemconfigure main -fill $color
            $c itemconfigure hilite -outline \
                [Rappture::color::brightness $color 1.0]
            $c itemconfigure lolite -outline \
                [Rappture::color::brightness $color -0.3]
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _resize
#
# Used internally to resize the internal canvas based on the -onimage
# or -offimage options, or the size of the text.
# ----------------------------------------------------------------------
itcl::body Rappture::Switch::_resize {} {
    if {$itk_option(-width) > 0} {
        set w $itk_option(-width)
    } else {
        set w 0
        foreach opt {-onimage -offimage} {
            if {$itk_option($opt) != ""} {
                set wi [expr {[image width $itk_option($opt)]+4}]
                if {$wi > $w} { set w $wi }
            }
        }
        if {$w <= 0} {
            set w [winfo reqheight $itk_component(value)]
        }
    }

    if {$itk_option(-height) > 0} {
        set h $itk_option(-height)
    } else {
        set h 0
        foreach opt {-onimage -offimage} {
            if {$itk_option($opt) != ""} {
                set hi [expr {[image height $itk_option($opt)]+4}]
                if {$hi > $h} { set h $hi }
            }
        }
        if {$h <= 0} {
            set h [winfo reqheight $itk_component(value)]
        }
    }

    $itk_component(icon) configure -width $w -height $h
}

# ----------------------------------------------------------------------
# USAGE: _hilite <component> <state>
#
# Used internally to resize the internal canvas based on the -onimage
# and -offimage options or the size of the text.
# ----------------------------------------------------------------------
itcl::body Rappture::Switch::_hilite {comp state} {
    if {$comp == "value"} {
        if {$state} {
            $itk_component(value) configure -relief solid
        } else {
            $itk_component(value) configure -relief flat
        }
        return
    }

    if {$state} {
        $itk_component($comp) configure -relief solid
    } else {
        $itk_component($comp) configure -relief flat
    }
}

# ----------------------------------------------------------------------
# USAGE: _presets post
# USAGE: _presets unpost
# USAGE: _presets select
#
# Used internally to handle the list of presets for this gauge.  The
# post/unpost options are invoked when the list is posted or unposted
# to manage the relief of the controlling button.  The select option
# is invoked whenever there is a selection from the list, to assign
# the value back to the gauge.
# ----------------------------------------------------------------------
itcl::body Rappture::Switch::_presets {option} {
    switch -- $option {
        post {
            set i [$itk_component(presetlist) index $_value]
            if {$i >= 0} {
                $itk_component(presetlist) select clear 0 end
                $itk_component(presetlist) select set $i
            }
            after 10 [list $itk_component(presets) configure -relief sunken]
        }
        unpost {
            $itk_component(presets) configure -relief flat
        }
        select {
            set val [$itk_component(presetlist) current]
            if {"" != $val} {
                value $val
            }
        }
        default {
            error "bad option \"$option\": should be post, unpost, select"
        }
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -onimage
# ----------------------------------------------------------------------
itcl::configbody Rappture::Switch::onimage {
    if {$itk_option(-onimage) != ""
          && [catch {image width $itk_option(-onimage)}]} {
        error "bad value \"$itk_option(-onimage)\": should be Tk image"
    }
    _resize

    if {$_value} {
        # if the off state? then redraw to show this change
        _redraw
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -offimage
# ----------------------------------------------------------------------
itcl::configbody Rappture::Switch::offimage {
    if {$itk_option(-offimage) != ""
          && [catch {image width $itk_option(-offimage)}]} {
        error "bad value \"$itk_option(-offimage)\": should be Tk image"
    }
    _resize

    if {!$_value} {
        # if the off state? then redraw to show this change
        _redraw
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -valueposition
# ----------------------------------------------------------------------
itcl::configbody Rappture::Switch::valueposition {
    array set side2anchor {
        left   e
        right  w
        top    s
        bottom n
    }
    set pos $itk_option(-valueposition)
    if {![info exists side2anchor($pos)]} {
        error "bad value \"$pos\": should be [join [lsort [array names side2anchor]] {, }]"
    }
    pack $itk_component(vframe) -before $itk_component(icon) \
        -side $pos -expand yes -fill both -ipadx 2
    $itk_component(value) configure -anchor $side2anchor($pos)

    set s [expr {($pos == "left") ? "left" : "right"}]
    pack $itk_component(presets) -before $itk_component(value) \
        -side $s -fill y
}
