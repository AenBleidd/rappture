# ----------------------------------------------------------------------
#  COMPONENT: switch - on/off switch
#
#  This widget is used to control a (boolean) on/off value.  
# It is just a wrapper around a checkbutton.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

itcl::class Rappture::Switch {
    inherit itk::Widget

    itk_option define -oncolor onColor Color "green"
    itk_option define -state state State "normal"

    constructor {args} { # defined below }
    public method value {args}
    private variable _value 0  ;# value for this widget
}
                                                                                
itk::usual Switch {
    keep -cursor -background
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Switch::constructor {args} {

    itk_component add value {
        checkbutton $itk_interior.value  -variable [itcl::scope _value]
    } {
        rename -background -textbackground textBackground Background
    }
    pack $itk_component(value) -side left -expand yes -fill both
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
        set newval [expr {($newval) ? 1 : 0}]
        if {$onlycheck} {
            return
        }
        set _value $newval
        event generate $itk_component(hull) <<Value>>

    } elseif {[llength $args] != 0} {
        error "wrong # args: should be \"value ?-check? ?newval?\""
    }
    return [expr {($_value) ? "yes" : "no"}]
}


# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -oncolor
# ----------------------------------------------------------------------
itcl::configbody Rappture::Switch::oncolor {
    $itk_component(value) configure -selectcolor $itk_option(-oncolor) 
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -state
# ----------------------------------------------------------------------
itcl::configbody Rappture::Switch::state {
    set valid {normal disabled}
    if {[lsearch -exact $valid $itk_option(-state)] < 0} {
        error "bad value \"$itk_option(-state)\": should be [join $valid {, }]"
    }
    $itk_component(value) configure -state $itk_option(-state)
}
