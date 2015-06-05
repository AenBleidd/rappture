# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: combochecks - like a combobox, but items with checkboxes
#
#  This widget looks a lot like a combobox, but has a drop-down list
#  of checkbox items.  The list of checked items is shown in the
#  entry area as a comma-separated list.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require BLT

option add *Combochecks.borderWidth 2 widgetDefault
option add *Combochecks.relief sunken widgetDefault
option add *Combochecks.width 10 widgetDefault
option add *Combochecks.textBackground white widgetDefault
option add *Combochecks.textForeground black widgetDefault
option add *Combochecks.disabledBackground white widgetDefault
option add *Combochecks.disabledForeground gray widgetDefault
option add *Combochecks.font -*-helvetica-medium-r-normal-*-12-* widgetDefault

itcl::class Rappture::Combochecks {
    inherit itk::Widget

    itk_option define -state state State "normal"
    itk_option define -width width Width 0
    itk_option define -disabledbackground disabledBackground DisabledBackground ""
    itk_option define -disabledforeground disabledForeground DisabledForeground ""

    constructor {args} { # defined below }

    public method value {args}
    public method current {}
    public method choices {option args}

    protected method _entry {option}
    protected method _fixDisplay {}
    protected method _fixState {}

    blt::bitmap define CombochecksArrow {
        #define arrow_width 8
        #define arrow_height 4
        static unsigned char arrow_bits[] = {
           0xfe, 0x7c, 0x38, 0x10};
    }
    private variable _value2label
    private variable _label2value
}

itk::usual Combochecks {
    keep -cursor -font
    keep -foreground -background
    keep -textforeground -textbackground
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Combochecks::constructor {args} {
    itk_option add hull.borderwidth hull.relief

    itk_component add button {
        button $itk_interior.btn -bitmap CombochecksArrow -padx 0 \
            -borderwidth 1 -relief raised -highlightthickness 0
    } {
        usual
        ignore -highlightthickness -highlightbackground -highlightcolor
        ignore -borderwidth -relief
    }
    pack $itk_component(button) -side right -fill y

    itk_component add entry {
        entry $itk_interior.entry -borderwidth 0 -relief flat -state disabled
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

    bind $itk_component(entry) <KeyPress-Return> \
        [itcl::code $this _entry apply]
    bind $itk_component(entry) <ButtonPress> \
        [itcl::code $this _entry click]

    itk_component add ddlist {
        Rappture::Dropdownchecks $itk_component(button).ddlist
    }
    bind $itk_component(ddlist) <<DropdownchecksSelect>> \
        [itcl::code $this _fixDisplay]

    $itk_component(button) configure -command \
        [list $itk_component(ddlist) post $itk_component(hull) left]

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
itcl::body Rappture::Combochecks::value {args} {
    switch -- [llength $args] {
        0 {
            # return a list of checked optons
            set max [$itk_component(ddlist) size]
            set rlist ""
            foreach val [$itk_component(ddlist) get -value] {
                if {[$itk_component(ddlist) state $val]} {
                    lappend rlist $val
                }
            }
            return $rlist
        }
        1 {
            set newval [lindex $args 0]

            $itk_component(ddlist) reset
            foreach part $newval {
                $itk_component(ddlist) state $part 1
            }
            _fixDisplay

            after 10 [list catch \
                [list event generate $itk_component(hull) <<Value>>]]

        }
        default {
            error "wrong # args: should be \"value ?newval?\""
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: choices insert <pos> ?<value1> <label1> ...?
# USAGE: choices delete <first> ?<last>?
# USAGE: choices get ?-value|-label|-both? ?<index>?
# USAGE: choices state <value> ?on|off?
# USAGE: choices index <value>
#
# Clients use this to manipulate the list of choices in the drop-down
# list.  Each choice is represented by a (computer-friendly) value
# and its corresponding (human-friendly) label.  The "get" option
# returns information about options on the list, including the value,
# the label, or both.
# ----------------------------------------------------------------------
itcl::body Rappture::Combochecks::choices {option args} {
    eval $itk_component(ddlist) $option $args
}

# ----------------------------------------------------------------------
# USAGE: _entry apply
# USAGE: _entry click
#
# Used internally to handle the dropdown list for this combobox.  The
# post/unpost options are invoked when the list is posted or unposted
# to manage the relief of the controlling button.  The select option
# is invoked whenever there is a selection from the list, to assign
# the value back to the gauge.
# ----------------------------------------------------------------------
itcl::body Rappture::Combochecks::_entry {option} {
    switch -- $option {
        apply {
            if {$itk_option(-state) == "normal"} {
                event generate $itk_component(hull) <<Value>>
            }
        }
        click {
            if {$itk_option(-state) == "normal"} {
                $itk_component(button) configure -relief sunken
                update idletasks; after 100
                $itk_component(button) configure -relief raised

                $itk_component(ddlist) post $itk_component(hull) left
            }
        }
        default {
            error "bad option \"$option\": should be apply, click"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _fixDisplay
#
# Used internally to handle the dropdown list for this combobox.
# Invoked whenever there is a selection from the list, to assign
# the value back to the entry.
# ----------------------------------------------------------------------
itcl::body Rappture::Combochecks::_fixDisplay {} {
    $itk_component(entry) configure -state normal
    $itk_component(entry) delete 0 end
    $itk_component(entry) insert end [join [value] {, }]
    $itk_component(entry) configure -state disabled
}

# ----------------------------------------------------------------------
# USAGE: _fixState
#
# Used internally to fix the widget state when the -state option
# changes.
# ----------------------------------------------------------------------
itcl::body Rappture::Combochecks::_fixState {} {
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

    if {$itk_option(-state) != "normal"} {
        # can't keep focus here -- move it along to the next widget
        if {[focus] == $itk_component(entry)} {
            focus [tk_focusNext [focus]]
        }
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -state
# ----------------------------------------------------------------------
itcl::configbody Rappture::Combochecks::state {
    set valid {normal disabled}
    if {[lsearch -exact $valid $itk_option(-state)] < 0} {
        error "bad value \"$itk_option(-state)\": should be [join $valid {, }]"
    }
    _fixState
}
