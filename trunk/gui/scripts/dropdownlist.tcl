# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: dropdownlist - drop-down list of items
#
#  This is a drop-down listbox, which might be used in conjunction
#  with a combobox.
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

option add *Dropdownlist.textBackground white widgetDefault
option add *Dropdownlist.outline black widgetDefault
option add *Dropdownlist.borderwidth 1 widgetDefault
option add *Dropdownlist.relief flat widgetDefault
option add *Dropdownlist.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault

itcl::class Rappture::Dropdownlist {
    inherit Rappture::Dropdown

    constructor {args} { # defined below }

    public method insert {pos args}
    public method delete {first {last ""}}
    public method index {args}
    public method get {args}
    public method size {}
    public method select {option args}
    public method current {{what -value}}

    private variable _values ""  ;# values for all elems in list
    private variable _labels ""  ;# labels for each of the _values

    protected method _adjust {{widget ""}}
    protected method _react {}
}

itk::usual Dropdownlist {
    keep -background -outline -cursor -font
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Dropdownlist::constructor {args} {
    itk_component add scroller {
        Rappture::Scroller $itk_interior.sc \
            -xscrollmode off -yscrollmode auto
    }
    pack $itk_component(scroller) -expand yes -fill both

    itk_component add list {
        listbox $itk_component(scroller).list \
            -selectmode single -exportselection no \
            -highlightthickness 0
    } {
        usual
        rename -background -textbackground textBackground Background
        rename -foreground -textforeground textForeground Foreground
        ignore -highlightthickness -highlightbackground -highlightcolor
        keep -relief
    }
    $itk_component(scroller) contents $itk_component(list)

    # add bindings so the listbox can react to selections
    bind RapptureDropdownlist-$this <ButtonRelease-1> [itcl::code $this _react]
    bind RapptureDropdownlist-$this <KeyPress-Return> [itcl::code $this _react]
    bind RapptureDropdownlist-$this <KeyPress-space> [itcl::code $this _react]
    bind RapptureDropdownlist-$this <KeyPress-Escape> [itcl::code $this unpost]

    set btags [bindtags $itk_component(list)]
    set i [lsearch $btags [winfo class $itk_component(list)]]
    if {$i < 0} {
        set i end
    }
    set btags [linsert $btags [expr {$i+1}] RapptureDropdownlist-$this]
    bindtags $itk_component(list) $btags

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# USAGE: insert <pos> ?<value1> <label1> <value2> <label2> ...?
#
# Inserts one or more values into this drop-down list.  Each value
# has a keyword (computer-friendly value) and a label (human-friendly
# value).  The labels appear in the listbox.  If the label is "--",
# then the value is used as the label.
# ----------------------------------------------------------------------
itcl::body Rappture::Dropdownlist::insert {pos args} {
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
        $itk_component(list) insert $pos $label
        incr pos
    }
}

# ----------------------------------------------------------------------
# USAGE: delete <first> ?<last>?
#
# Deletes one or more values from this drop-down list.  The indices
# <first> and <last> should be integers from 0 or the keyword "end".
# All values in the specified range are deleted.
# ----------------------------------------------------------------------
itcl::body Rappture::Dropdownlist::delete {first {last ""}} {
    if {$last == ""} {
        set last $first
    }
    if {![regexp {^[0-9]+|end$} $first]} {
        error "bad index \"$first\": should be integer or \"end\""
    }
    if {![regexp {^[0-9]+|end$} $last]} {
        error "bad index \"$last\": should be integer or \"end\""
    }

    set _values [lreplace $_values $first $last]
    set _labels [lreplace $_labels $first $last]
    $itk_component(list) delete $first $last
}

# ----------------------------------------------------------------------
# USAGE: index ?-value|-label? <value>
#
# Returns the integer index for the position of the specified <value>
# in the list.  Returns -1 if the value is not recognized.
# ----------------------------------------------------------------------
itcl::body Rappture::Dropdownlist::index {args} {
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
itcl::body Rappture::Dropdownlist::get {args} {
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
itcl::body Rappture::Dropdownlist::size {} {
    return [llength $_values]
}

# ----------------------------------------------------------------------
# USAGE: select <option> ?<arg> ...?
#
# Used to manipulate the selection in the listbox.  All options and
# args are passed along to the underlying listbox.
# ----------------------------------------------------------------------
itcl::body Rappture::Dropdownlist::select {option args} {
    if {$option == "set"} {
        $itk_component(list) activate [lindex $args 0]
    }
    eval $itk_component(list) selection $option $args
}

# ----------------------------------------------------------------------
# USAGE: current ?-value|-label|-both?
#
# Clients use this to query the current selection from the listbox.
# Returns the value, label, or both, according to the option.
# ----------------------------------------------------------------------
itcl::body Rappture::Dropdownlist::current {{what -value}} {
    set i [$itk_component(list) curselection]
    if {$i != ""} {
        switch -- $what {
            -value { return [lindex $_values $i] }
            -label { return [lindex $_labels $i] }
            -both  { return [list [lindex $_values $i] [lindex $_labels $i]] }
            default {
                error "bad option \"$what\": should be -value, -label, -both"
            }
        }
    }
    return ""
}

# ----------------------------------------------------------------------
# USAGE: _adjust ?<widget>?
#
# This method is invoked each time the dropdown is posted to adjust
# its size and contents.  If the <widget> is specified, it is the
# controlling widget.
# ----------------------------------------------------------------------
itcl::body Rappture::Dropdownlist::_adjust {{widget ""}} {
    chain $widget

    set fnt [$itk_component(list) cget -font]
    set maxw 0
    foreach str $_labels {
        set w [font measure $fnt $str]
        if {$w > $maxw} { set maxw $w }
    }
    if {$widget != ""} {
        if {$maxw < [winfo width $widget]} { set maxw [winfo width $widget] }
    }
    set avg [font measure $fnt "n"]
    $itk_component(list) configure -width [expr {round($maxw/double($avg))+1}]

    if {$widget != ""} {
        set y [expr {[winfo rooty $widget]+[winfo height $widget]}]
        set h [font metrics $fnt -linespace]
        set lines [expr {double([winfo screenheight $widget]-$y)/$h}]
        if {[llength $_labels] < $lines} {
            $itk_component(list) configure -height [llength $_labels]
        } else {
            $itk_component(list) configure -height 10
        }
    }

    focus $itk_component(list)
}

# ----------------------------------------------------------------------
# USAGE: _react
#
# Invoked automatically when the user has selected something from
# the listbox.  Unposts the drop-down and generates an event letting
# the client know that the selection has changed.
# ----------------------------------------------------------------------
itcl::body Rappture::Dropdownlist::_react {} {
    unpost
    event generate $itk_component(hull) <<DropdownlistSelect>>
}
