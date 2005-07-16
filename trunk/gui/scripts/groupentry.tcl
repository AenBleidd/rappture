# ----------------------------------------------------------------------
#  COMPONENT: GroupEntry - widget containing a group of controls
#
#  This widget represents a <group> entry on a control panel.
#  It contains a series of other controls.  Sort of a glorified
#  frame widget.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005
#  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Itk

itcl::class Rappture::GroupEntry {
    inherit itk::Widget

    constructor {owner path args} { # defined below }

    public method value {args}

    public method label {}
    public method tooltip {}

    private variable _owner ""    ;# thing managing this control
    private variable _path ""     ;# path in XML to this number
}

itk::usual GroupEntry {
    keep -cursor -font
    keep -foreground -background
    keep -textbackground
    keep -selectbackground -selectforeground -selectborderwidth
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::GroupEntry::constructor {owner path args} {
    if {[catch {$owner isa Rappture::ControlOwner} valid] != 0 || !$valid} {
        error "bad object \"$owner\": should be Rappture::ControlOwner"
    }
    set _owner $owner
    set _path $path

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
itcl::body Rappture::GroupEntry::value {args} {
    # groups have no value
    return ""
}

# ----------------------------------------------------------------------
# USAGE: label
#
# Clients use this to query the label associated with this widget.
# Reaches into the XML and pulls out the appropriate label string.
# ----------------------------------------------------------------------
itcl::body Rappture::GroupEntry::label {} {
    return [$_owner xml get $_path.about.label]
}

# ----------------------------------------------------------------------
# USAGE: tooltip
#
# Clients use this to query the tooltip associated with this widget.
# Reaches into the XML and pulls out the appropriate description
# string.  Returns the string that should be used with the
# Rappture::Tooltip facility.
# ----------------------------------------------------------------------
itcl::body Rappture::GroupEntry::tooltip {} {
    return [$_owner xml get $_path.about.description]
}
