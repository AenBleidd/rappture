# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: dropdownchecks - drop-down list of checkbox items
#
#  This is the drop-down for the Combochecks widget, which maintains
#  a list of options.
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

option add *Dropdownchecks.textBackground white widgetDefault
option add *Dropdownchecks.foreground black widgetDefault
option add *Dropdownchecks.outline black widgetDefault
option add *Dropdownchecks.borderwidth 1 widgetDefault
option add *Dropdownchecks.relief flat widgetDefault
option add *Dropdownchecks.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault

itcl::class Rappture::Dropdownchecks {
    inherit Rappture::Dropdown

    itk_option define -font font Font ""
    itk_option define -foreground foreground Foreground ""

    constructor {args} { # defined below }

    public method insert {pos args}
    public method delete {first {last ""}}
    public method index {args}
    public method get {args}
    public method size {}
    public method state {value {newval ""}}
    public method reset {}

    private variable _dispatcher "" ;# dispatcher for !events
    private variable _values ""     ;# values for all elems in list
    private variable _labels ""     ;# labels for each of the _values
    private variable _states        ;# maps list item => on/off state
    private variable _layout        ;# layout parameters for drawing

    protected method _redraw {args}
    protected method _layout {args}
    protected method _adjust {{widget ""}}
    protected method _react {x y}
}

itk::usual Dropdownchecks {
    keep -background -outline -cursor -font
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Dropdownchecks::constructor {args} {
    Rappture::dispatcher _dispatcher
    $_dispatcher register !redraw
    $_dispatcher dispatch $this !redraw [itcl::code $this _redraw]
    $_dispatcher register !layout
    $_dispatcher dispatch $this !layout [itcl::code $this _layout]

    itk_component add scroller {
        Rappture::Scroller $itk_interior.sc \
            -xscrollmode off -yscrollmode auto
    }
    pack $itk_component(scroller) -expand yes -fill both

    itk_component add list {
        canvas $itk_component(scroller).list \
            -highlightthickness 0 -width 3i -height 1.5i
    } {
        usual
        rename -background -textbackground textBackground Background
        ignore -highlightthickness -highlightbackground -highlightcolor
        keep -relief
    }
    $itk_component(scroller) contents $itk_component(list)

    # add bindings so the listbox can react to selections
    bind RapptureDropdownchecks-$this <ButtonRelease-1> \
        [itcl::code $this _react %x %y]
    bind RapptureDropdownchecks-$this <KeyPress-Escape> \
        [itcl::code $this unpost]

    set btags [bindtags $itk_component(list)]
    set i [lsearch $btags [winfo class $itk_component(list)]]
    set btags [lreplace $btags $i $i RapptureDropdownchecks-$this]
    bindtags $itk_component(list) $btags

    eval itk_initialize $args

    $_dispatcher event -idle !layout
}

# ----------------------------------------------------------------------
# USAGE: insert <pos> ?<value1> <label1> <value2> <label2> ...?
#
# Inserts one or more values into this drop-down list.  Each value
# has a keyword (computer-friendly value) and a label (human-friendly
# value).  The labels appear in the listbox.  If the label is "--",
# then the value is used as the label.
# ----------------------------------------------------------------------
itcl::body Rappture::Dropdownchecks::insert {pos args} {
    if {"end" == $pos} {
        set pos [llength $_values]
    } elseif {![string is integer -strict $pos]} {
        error "bad index \"$pos\": should be integer or \"end\""
    }

    if {[llength $args] == 1} {
        set args [lindex $args 0]
    }
    if {[llength $args] % 2 != 0} {
        error "wrong # args: should be \"insert pos ?value label ...?\""
    }

    foreach {val label} $args {
        if {$label == "--"} {
            set label $val
        }
        set _values [linsert $_values $pos $val]
        set _labels [linsert $_labels $pos $label]
        set _states($val) 0
        incr pos
    }
    $_dispatcher event -idle !redraw
}

# ----------------------------------------------------------------------
# USAGE: delete <first> ?<last>?
#
# Deletes one or more values from this drop-down list.  The indices
# <first> and <last> should be integers from 0 or the keyword "end".
# All values in the specified range are deleted.
# ----------------------------------------------------------------------
itcl::body Rappture::Dropdownchecks::delete {first {last ""}} {
    if {$last == ""} {
        set last $first
    }
    if {![regexp {^[0-9]+|end$} $first]} {
        error "bad index \"$first\": should be integer or \"end\""
    }
    if {![regexp {^[0-9]+|end$} $last]} {
        error "bad index \"$last\": should be integer or \"end\""
    }

    foreach val [lrange $_values $first $last] {
        unset _states($val)
    }
    set _values [lreplace $_values $first $last]
    set _labels [lreplace $_labels $first $last]

    $_dispatcher event -idle !redraw
}

# ----------------------------------------------------------------------
# USAGE: index ?-value|-label? <value>
#
# Returns the integer index for the position of the specified <value>
# in the list.  Returns -1 if the value is not recognized.
# ----------------------------------------------------------------------
itcl::body Rappture::Dropdownchecks::index {args} {
    set format -value
    set first [lindex $args 0]
    if {$first == "-value" || $first == "-label"} {
        set format $first
        set args [lrange $args 1 end]
    } elseif {[llength $args] > 1} {
        error "bad option \"$first\": should be -value or -label"
    }
    if {[llength $args] != 1} {
        error "wrong # args: should be \"index ?-value? ?-label? string\""
    }
    set value [lindex $args 0]

    switch -- $format {
        -value { return [lsearch -exact $_values $value] }
        -label { return [lsearch -exact $_labels $value] }
    }
    return -1
}

# ----------------------------------------------------------------------
# USAGE: get ?-value|-label|-both? ?<index>?
#
# Queries one or more values from the drop-down list.  With no args,
# it returns a list of all values and labels in the list.  If the
# index is specified, then it returns the value or label (or both)
# for the specified index.
# ----------------------------------------------------------------------
itcl::body Rappture::Dropdownchecks::get {args} {
    set format -both
    set first [lindex $args 0]
    if {[string index $first 0] == "-"} {
        set choices {-value -label -both}
        if {[lsearch $choices $first] < 0} {
            error "bad option \"$first\": should be [join [lsort $choices] {, }]"
        }
        set format $first
        set args [lrange $args 1 end]
    }

    # return the whole list or just a single value
    if {[llength $args] > 1} {
        error "wrong # args: should be \"get ?-value|-label|-both? ?index?\""
    }
    if {[llength $args] == 0} {
        set vlist $_values
        set llist $_labels
        set op lappend
    } else {
        set i [lindex $args 0]
        set vlist [list [lindex $_values $i]]
        set llist [list [lindex $_labels $i]]
        set op set
    }

    # scan through and build up the return list
    set rlist ""
    foreach v $vlist l $llist {
        switch -- $format {
            -value { $op rlist $v }
            -label { $op rlist $l }
            -both  { lappend rlist $v $l }
        }
    }
    return $rlist
}

# ----------------------------------------------------------------------
# USAGE: size
#
# Returns the number of choices in this list.
# ----------------------------------------------------------------------
itcl::body Rappture::Dropdownchecks::size {} {
    return [llength $_values]
}

# ----------------------------------------------------------------------
# USAGE: state <value> ?on|off?
#
# Used to query or set the state for the underlying values of the
# listbox.  The <value> is the value associated with a label option.
# With no other arg, it returns the current state for that value.
# Otherwise, it sets the state for that value.
# ----------------------------------------------------------------------
itcl::body Rappture::Dropdownchecks::state {val {newval ""}} {
    if {$newval eq ""} {
        if {[info exists _states($val)]} {
            return $_states($val)
        }
        return ""
    }

    if {[info exists _states($val)]} {
        if {$newval} {
            set img [Rappture::icon cbon]
        } else {
            set img [Rappture::icon cboff]
        }
        $itk_component(list) itemconfigure box-$val -image $img
        set _states($val) $newval
    } else {
        error "bad value \"$val\": should be one of [join $_values {, }]"
    }
}

# ----------------------------------------------------------------------
# USAGE: reset
#
# Resets the state of all checkboxes back to 0 (unchecked).
# ----------------------------------------------------------------------
itcl::body Rappture::Dropdownchecks::reset {} {
    foreach val $_values {
        set _states($val) 0
    }
}

# ----------------------------------------------------------------------
# USAGE: _layout ?<eventData>?
#
# Used internally to recompute layout parameters whenever the font
# changes.
# ----------------------------------------------------------------------
itcl::body Rappture::Dropdownchecks::_layout {args} {
    # figure out the sizes of the checkboxes
    set wmax 0
    set hmax 0
    foreach icon {cbon cboff} {
        set img [Rappture::icon $icon]
        set w [image width $img]
        if {$w > $wmax} {set wmax $w}
        set h [image height $img]
        if {$h > $hmax} {set hmax $h}
    }
    incr wmax 2

    set fnt $itk_option(-font)
    set lineh [expr {[font metrics $fnt -linespace]+2}]

    if {$hmax > $lineh} {
        set lineh $hmax
    }

    set _layout(boxwidth) $wmax
    set _layout(lineh) $lineh
}

# ----------------------------------------------------------------------
# USAGE: _redraw ?<eventData>?
#
# Used internally to redraw the items in the list whenever new items
# are added or removed.
# ----------------------------------------------------------------------
itcl::body Rappture::Dropdownchecks::_redraw {args} {
    set c $itk_component(list)
    $c delete all

    set fg $itk_option(-foreground)

    if {[$_dispatcher ispending !layout]} {
        $_dispatcher event -now !layout
    }

    set y 0
    set _layout(ypos) ""
    for {set i 0} {$i < [llength $_values]} {incr i} {
        set x0 2
        set val [lindex $_values $i]
        set label [lindex $_labels $i]

        if {$_states($val)} {
            set img [Rappture::icon cbon]
        } else {
            set img [Rappture::icon cboff]
        }

        set ymid [expr {$y + $_layout(lineh)/2}]
        $c create image $x0 $ymid -anchor w -image $img -tags box-$val
        incr x0 $_layout(lineh)
        $c create text $x0 $ymid -anchor w -text $label -fill $fg

        lappend _layout(ypos) [incr y $_layout(lineh)]
    }

    # fix the overall size for scrolling
    foreach {x0 y0 x1 y1} [$c bbox all] break
    $c configure -scrollregion [list 0 0 $x1 $y1]
}

# ----------------------------------------------------------------------
# USAGE: _adjust ?<widget>?
#
# This method is invoked each time the dropdown is posted to adjust
# its size and contents.  If the <widget> is specified, it is the
# controlling widget.
# ----------------------------------------------------------------------
itcl::body Rappture::Dropdownchecks::_adjust {{widget ""}} {
    chain $widget

    set c $itk_component(list)
    set lineh $_layout(lineh)

    foreach {x0 y0 x1 y1} [$c bbox all] break
    set maxw [expr {$x1+2}]

    set maxh [expr {10*$lineh + 2*[$c cget -borderwidth] + 2}]

    if {$widget != ""} {
        if {$maxw < [winfo width $widget]} { set maxw [winfo width $widget] }
    }
    $c configure -width $maxw -height $maxh

    if {$widget != ""} {
        set y [expr {[winfo rooty $widget]+[winfo height $widget]}]
        set screenh [winfo screenheight $widget]
        set lines [expr {round(floor(double($screenh-$y)/$lineh))}]
        if {[llength $_labels] < $lines} {
            set lines [llength $_labels]
        }
        set maxh [expr {[lindex $_layout(ypos) [expr {$lines-1}]]+2}]
        $c configure -height $maxh
    }

    focus $c
}

# ----------------------------------------------------------------------
# USAGE: _react <x> <y>
#
# Invoked automatically when the user has selected something from
# the listbox.  Unposts the drop-down and generates an event letting
# the client know that the selection has changed.
# ----------------------------------------------------------------------
itcl::body Rappture::Dropdownchecks::_react {x y} {
    if {$x >= 0 && $x <= [winfo width $itk_component(hull)]
          && $y >= 0 && $y <= [winfo height $itk_component(hull)]} {

        set x [$itk_component(list) canvasx $x]
        set y [$itk_component(list) canvasy $y]

        for {set i 0} {$i < [llength $_values]} {incr i} {
            if {$y < [lindex $_layout(ypos) $i]} {
                set val [lindex $_values $i]
                if {$_states($val)} {
                    state $val 0
                } else {
                    state $val 1
                }
                break
            }
        }
        event generate $itk_component(hull) <<DropdownchecksSelect>>
    } else {
        unpost
    }
}
