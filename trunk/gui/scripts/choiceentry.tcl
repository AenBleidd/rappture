# ----------------------------------------------------------------------
#  COMPONENT: ChoiceEntry - widget for entering a choice of strings
#
#  This widget represents a <choice> entry on a control panel.
#  It is used to choose one of several mutually-exclusive strings.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005
#  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Itk

itcl::class Rappture::ChoiceEntry {
    inherit itk::Widget

    constructor {xmlobj path args} { # defined below }

    public method value {args}

    public method label {}
    public method tooltip {}

    protected method _newValue {}
    protected method _tooltip {}

    private variable _xmlobj ""   ;# XML containing description
    private variable _path ""     ;# path in XML to this number
}

itk::usual ChoiceEntry {
    keep -cursor -font
    keep -foreground -background
    keep -textforeground -textbackground
    keep -selectbackground -selectforeground -selectborderwidth
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::ChoiceEntry::constructor {xmlobj path args} {
    if {![Rappture::library isvalid $xmlobj]} {
        error "bad value \"$xmlobj\": should be Rappture::library"
    }
    set _xmlobj $xmlobj
    set _path $path

    #
    # Create the widget and configure it properly based on other
    # hints in the XML.
    #
    itk_component add choice {
        Rappture::Combobox $itk_interior.choice -editable no
    }
    pack $itk_component(choice) -expand yes -fill both
    bind $itk_component(choice) <<Value>> [itcl::code $this _newValue]

    eval itk_initialize $args

    #
    # Plug in the various choices for this widget.
    #
    # plug in the various options for the choice
    set max 10
    foreach cname [$xmlobj children -type option $path] {
        set str [string trim [$xmlobj get $path.$cname.label]]
        if {"" != $str} {
            $itk_component(choice) choices insert end $path.$cname $str
            set len [string length $str]
            if {$len > $max} { set max $len }
        }
    }
    $itk_component(choice) configure -width $max

    #
    # Assign the default value to this widget, if there is one.
    #
    set str [$xmlobj get $path.default]
    if {"" != $str != ""} { $itk_component(choice) value $str }
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
itcl::body Rappture::ChoiceEntry::value {args} {
    set onlycheck 0
    set i [lsearch -exact $args -check]
    if {$i >= 0} {
        set onlycheck 1
        set args [lreplace $args $i $i]
    }

    if {[llength $args] == 1} {
        if {$onlycheck} {
            # someday we may add validation...
            return
        }
        set newval [lindex $args 0]
        $itk_component(choice) value $newval
        return $newval

    } elseif {[llength $args] != 0} {
        error "wrong # args: should be \"value ?-check? ?newval?\""
    }

    #
    # Query the value and return.
    #
    return [$itk_component(choice) value]
}

# ----------------------------------------------------------------------
# USAGE: label
#
# Clients use this to query the label associated with this widget.
# Reaches into the XML and pulls out the appropriate label string.
# ----------------------------------------------------------------------
itcl::body Rappture::ChoiceEntry::label {} {
    set label [$_xmlobj get $_path.about.label]
    if {"" == $label} {
        set label "Number"
    }
    return $label
}

# ----------------------------------------------------------------------
# USAGE: tooltip
#
# Clients use this to query the tooltip associated with this widget.
# Reaches into the XML and pulls out the appropriate description
# string.  Returns the string that should be used with the
# Rappture::Tooltip facility.
# ----------------------------------------------------------------------
itcl::body Rappture::ChoiceEntry::tooltip {} {
    # query tooltip on-demand based on current choice
    return "@[itcl::code $this _tooltip]"
}

# ----------------------------------------------------------------------
# USAGE: _newValue
#
# Invoked automatically whenever the value in the choice changes.
# Sends a <<Value>> event to notify clients of the change.
# ----------------------------------------------------------------------
itcl::body Rappture::ChoiceEntry::_newValue {} {
    event generate $itk_component(hull) <<Value>>
}

# ----------------------------------------------------------------------
# USAGE: _tooltip
#
# Returns the tooltip for this widget, given the current choice in
# the selector.  This is normally called by the Rappture::Tooltip
# facility whenever it is about to pop up a tooltip for this widget.
# ----------------------------------------------------------------------
itcl::body Rappture::ChoiceEntry::_tooltip {} {
    set tip [string trim [$_xmlobj get $_path.about.description]]

    # get the description for the current choice, if there is one
    set str [$itk_component(choice) value]
    set path [$itk_component(choice) translate $str]

    if {"" != $str} {
        append tip "\n\n$str:"

        if {$path != ""} {
            set desc [$_xmlobj get $path.description]
            if {[string length $desc] > 0} {
                append tip "\n$desc"
            }
        }
    }
    return $tip
}
