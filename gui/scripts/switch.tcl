# ----------------------------------------------------------------------
#  COMPONENT: switch - on/off switch
#
#  This widget is used to control a (boolean) on/off value.  
# It is just a wrapper around a button.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

itcl::class Rappture::Switch {
    inherit itk::Widget

    itk_option define -oncolor onColor Color "#00cc00"
    itk_option define -state state State "normal"
    itk_option define -showimage showimage Showimage "true"
    itk_option define -showtext showtext Showtext "true"

    constructor {args} { # defined below }
    public method value {args}
    public method updateText {}
    private method _toggle {args}
    private variable _value 0  ;# value for this widget
}

itk::usual Switch {
    keep -cursor -background
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Switch::constructor {args} {

    itk_component add button {
        button $itk_interior.value \
            -compound left \
            -overrelief flat -relief flat -padx 3 -pady 0 -bd 0 \
            -command [itcl::code $this _toggle]
    } {
        #rename -background -textbackground textBackground Background
    }
    pack $itk_component(button) -side left -expand yes -fill both
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
        updateText
    } elseif {[llength $args] != 0} {
        error "wrong # args: should be \"value ?-check? ?newval?\""
    }
    return [expr {($_value) ? "yes" : "no"}]
}

# ----------------------------------------------------------------------
# _toggle
#
#        Use internally to convert the toggled button into the
#        proper boolean format.  Yes, right now it's hardcoded to 
#        yes/no.  But in the future it could be some other text.
#
#        Can't use old "value" method because _value is already set
#        be the widget and doesn't pass the value on the command line.
#
# ----------------------------------------------------------------------
itcl::body Rappture::Switch::_toggle {} {
    set _value [expr ($_value==0) ]
    event generate $itk_component(hull) <<Value>>
    updateText
}

itcl::body Rappture::Switch::updateText {} {
    set image ""
    set text ""
    if { $_value } {
        if {$itk_option(-showimage)} {
            set image "[Rappture::icon cbon]"
        }
        if {$itk_option(-showtext)} {
            set text "yes"
        }
    } else {
        if {$itk_option(-showimage)} {
            set image "[Rappture::icon cboff]"
        }
        if {$itk_option(-showtext)} {
            set text "no"
        }
    }
    $itk_component(button) configure -text $text -image $image
}


# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -oncolor
# ----------------------------------------------------------------------
itcl::configbody Rappture::Switch::oncolor {
    #$itk_component(button) configure -selectcolor $itk_option(-oncolor) 
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -state
# ----------------------------------------------------------------------
itcl::configbody Rappture::Switch::state {
    set valid {normal disabled}
    if {[lsearch -exact $valid $itk_option(-state)] < 0} {
        error "bad value \"$itk_option(-state)\": should be [join $valid {, }]"
    }
    $itk_component(button) configure -state $itk_option(-state)
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -showimage
# ----------------------------------------------------------------------
itcl::configbody Rappture::Switch::showimage {
    if {[string is boolean $itk_option(-showimage)] != 1} {
        error "bad value \"$itk_option(-showimage)\": should be a boolean"
    }
    updateText
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -showtext
# ----------------------------------------------------------------------
itcl::configbody Rappture::Switch::showtext {
    if {[string is boolean $itk_option(-showtext)] != 1} {
        error "bad value \"$itk_option(-showtext)\": should be a boolean"
    }
    updateText
}
