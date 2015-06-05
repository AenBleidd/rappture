# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: PeriodicElement - entry widget with a drop-down periodic
#             table.
#
#  This widget is a typical periodicelement, an entry widget with a drop-down
#  list of values.  If the -editable option is turned off, then the
#  value can be set only from the drop-down list.  Otherwise, the
#  drop-down is treated as a list of preset choices, but the user can
#  type anything in the entry area.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require BLT

option add *PeriodicElement.borderWidth 2 widgetDefault
option add *PeriodicElement.relief sunken widgetDefault
option add *PeriodicElement.width 10 widgetDefault
option add *PeriodicElement.editable yes widgetDefault
option add *PeriodicElement.textBackground white widgetDefault
option add *PeriodicElement.textForeground black widgetDefault
option add *PeriodicElement.disabledBackground white widgetDefault
option add *PeriodicElement.disabledForeground gray widgetDefault
option add *PeriodicElement.font -*-helvetica-medium-r-normal-*-12-* widgetDefault

itcl::class Rappture::PeriodicElement {
    inherit itk::Widget

    itk_option define -editable editable Editable ""
    itk_option define -state state State "normal"
    itk_option define -width width Width 0
    itk_option define -disabledbackground disabledBackground DisabledBackground ""
    itk_option define -disabledforeground disabledForeground DisabledForeground ""

    constructor {args} { # defined below }

    public method value {args}
    public method label {value}
    public method element {option args}

    protected method _entry {option}
    protected method _dropdown {option}
    protected method _fixState {}

    blt::bitmap define PeriodicElementArrow {
        #define arrow_width 8
        #define arrow_height 4
        static unsigned char arrow_bits[] = {
           0xfe, 0x7c, 0x38, 0x10};
    }

    private variable _lastValue ""
}

itk::usual PeriodicElement {
    keep -cursor -font
    keep -foreground -background
    keep -textforeground -textbackground
    keep -selectbackground -selectforeground -selectborderwidth
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::PeriodicElement::constructor {args} {
    itk_option add hull.borderwidth hull.relief

    itk_component add button {
        button $itk_interior.btn -bitmap PeriodicElementArrow -padx 0 \
            -borderwidth 1 -relief raised -highlightthickness 0
    } {
        usual
        ignore -highlightthickness -highlightbackground -highlightcolor
        ignore -borderwidth -relief
    }
    pack $itk_component(button) -side right -fill y

    itk_component add entry {
        entry $itk_interior.entry -borderwidth 0 -relief flat
    } {
        usual
        keep -width
        rename -highlightbackground -textbackground textBackground Background
        rename -background -textbackground textBackground Background
        rename -foreground -textforeground textForeground Foreground
        rename -disabledbackground -textbackground textBackground Background
        rename -disabledforeground -textforeground textForeground Foreground
        ignore -borderwidth -relief
    }
    pack $itk_component(entry) -side left -expand yes -fill both

    bind $itk_component(entry) <Return> \
        [itcl::code $this _entry apply]
    bind $itk_component(entry) <KP_Enter> \
        [itcl::code $this _entry apply]
    bind $itk_component(entry) <Tab> \
        [itcl::code $this _entry apply]
    bind $itk_component(entry) <ButtonPress> \
        [itcl::code $this _entry click]

    itk_component add ptable {
        Rappture::PeriodicTable $itk_component(button).ptable \
            -postcommand [itcl::code $this _dropdown post] \
            -unpostcommand [itcl::code $this _dropdown unpost] \
    }

    bind $itk_component(ptable) <<PeriodicTableSelect>> \
        [itcl::code $this _dropdown select]

    $itk_component(button) configure -command \
        [list $itk_component(ptable) post $itk_component(hull) left]

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# USAGE: value ?<newval>?
#
# Clients use this to query/set the value for this widget.  With
# no args, it returns the current value for the widget.  If the
# <newval> is specified, it sets the value of the widget and
# sends a <<Value>> event.
# ----------------------------------------------------------------------
itcl::body Rappture::PeriodicElement::value {args} {
    if {[llength $args] == 1} {
        set value [lindex $args 0]
    } elseif { [llength $args] == 0 }  {
        set value [$itk_component(entry) get]
    } else {
        error "wrong # args: should be \"value ?newval?\""
    }
    regsub -all -- "-" $value " " value
    if { [llength $value] > 1 } {
        set value [lindex $value 0]
    }
    set name [$itk_component(ptable) get -name $value]
    if { $name == "" } {
        set name $_lastValue
        bell
    }
    set symbol [$itk_component(ptable) get -symbol $name]
    if { $name != $_lastValue } {
        $itk_component(ptable) select $name
    }
    $itk_component(entry) configure -state normal
    $itk_component(entry) delete 0 end
    #$itk_component(entry) insert 0 "${symbol} - ${name}"
    $itk_component(entry) insert 0 "${name} - ${symbol}"
    if {!$itk_option(-editable)} {
        $itk_component(entry) configure -state disabled
    }
    set _lastValue $name
    if { [llength $args] == 1 } {
        after 10 \
            [list catch [list event generate $itk_component(hull) <<Value>>]]
    }
    return $name
}

# ----------------------------------------------------------------------
# USAGE: element include ?<elem>...?
# USAGE: element exclude ?<elem>...?
# USAGE: element get ?-weight|-name|-symbol|-number|-all? ?<elem>?
#
# Clients use this to manipulate the list of choices in the drop-down
# list.  Each choice is represented by a (computer-friendly) value
# and its corresponding (human-friendly) label.  The "get" option
# returns information about options on the list, including the value,
# the label, or both.
# ----------------------------------------------------------------------
itcl::body Rappture::PeriodicElement::element {option args} {
    eval $itk_component(ptable) $option $args
}

# ----------------------------------------------------------------------
# USAGE: _entry apply
# USAGE: _entry click
#
# Used internally to handle the dropdown list for this widget.  The
# post/unpost options are invoked when the list is posted or unposted
# to manage the relief of the controlling button.  The select option
# is invoked whenever there is a selection from the list, to assign
# the value back to the gauge.
# ----------------------------------------------------------------------
itcl::body Rappture::PeriodicElement::_entry {option} {
    switch -- $option {
        apply {
            if {$itk_option(-editable) && $itk_option(-state) == "normal"} {
                event generate $itk_component(hull) <<Value>>
            }
        }
        click {
            if {!$itk_option(-editable) && $itk_option(-state) == "normal"} {
                $itk_component(button) configure -relief sunken
                update idletasks; after 100
                $itk_component(button) configure -relief raised

                $itk_component(ptable) post $itk_component(hull) left
            }
        }
        default {
            error "bad option \"$option\": should be apply, click"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _dropdown post
# USAGE: _dropdown unpost
# USAGE: _dropdown select
#
# Used internally to handle the dropdown table for this widget.  The
# post/unpost options are invoked when the list is posted or unposted
# to manage the relief of the controlling button.  The select option
# is invoked whenever there is a selection from the list, to assign
# the value back to the gauge.
# ----------------------------------------------------------------------
itcl::body Rappture::PeriodicElement::_dropdown {option} {
    switch -- $option {
        post {
            set value [$itk_component(entry) get]
            if {$value != ""} {
                $itk_component(ptable) select $value
            }
        }
        unpost {
            if {$itk_option(-editable)} {
                focus $itk_component(entry)
            }
        }
        select {
            set value [$itk_component(ptable) get -name]
            if {"" != $value} {
                value $value
            }
        }
        default {
            error "bad option \"$option\": should be post, unpost, select"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _fixState
#
# Used internally to fix the widget state when the -editable/-state
# options change.
# ----------------------------------------------------------------------
itcl::body Rappture::PeriodicElement::_fixState {} {
    if {$itk_option(-state) == "normal"} {
        $itk_component(button) configure -state normal
        $itk_component(entry) configure \
            -background $itk_option(-textbackground) \
            -foreground $itk_option(-textforeground) \
            -disabledbackground $itk_option(-textbackground) \
            -disabledforeground $itk_option(-textforeground)
    } else {
        $itk_component(button) configure -state disabled
        $itk_component(entry) configure \
            -background $itk_option(-disabledbackground) \
            -foreground $itk_option(-disabledforeground) \
            -disabledbackground $itk_option(-disabledbackground) \
            -disabledforeground $itk_option(-disabledforeground)
    }

    if {$itk_option(-editable)} {
        if {$itk_option(-state) == "normal"} {
            $itk_component(entry) configure -state normal
        } else {
            $itk_component(entry) configure -state disabled
        }
    } else {
        $itk_component(entry) configure -state disabled
    }

    if {!$itk_option(-editable) || $itk_option(-state) != "normal"} {
        # can't keep focus here -- move it along to the next widget
        if {[focus] == $itk_component(entry)} {
            focus [tk_focusNext [focus]]
        }
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -editable
# ----------------------------------------------------------------------
itcl::configbody Rappture::PeriodicElement::editable {
    if {![string is boolean -strict $itk_option(-editable)]} {
        error "bad value \"$itk_option(-editable)\": should be boolean"
    }
    _fixState
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -state
# ----------------------------------------------------------------------
itcl::configbody Rappture::PeriodicElement::state {
    set valid {normal disabled}
    if {[lsearch -exact $valid $itk_option(-state)] < 0} {
        error "bad value \"$itk_option(-state)\": should be [join $valid {, }]"
    }
    _fixState
}
