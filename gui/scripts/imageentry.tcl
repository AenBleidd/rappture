# ----------------------------------------------------------------------
#  COMPONENT: ImageEntry - widget for displaying images
#
#  This widget represents an <image> entry on a control panel.
#  It displays images which help explain the input.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require Img

itcl::class Rappture::ImageEntry {
    inherit itk::Widget

    constructor {owner path args} { # defined below }

    public method value {args}

    public method label {}
    public method tooltip {}

    private variable _owner ""    ;# thing managing this control
    private variable _path ""     ;# path in XML to this image
}

itk::usual ImageEntry {
    keep -cursor -font
    keep -foreground -background
    keep -textbackground
    keep -selectbackground -selectforeground -selectborderwidth
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::ImageEntry::constructor {owner path args} {
    if {[catch {$owner isa Rappture::ControlOwner} valid] != 0 || !$valid} {
        error "bad object \"$owner\": should be Rappture::ControlOwner"
    }
    set _owner $owner
    set _path $path

    #
    # Create the widget and configure it properly based on other
    # hints in the XML.
    #
    itk_component add image {
        ::label $itk_interior.image
    }
    pack $itk_component(image) -expand yes -fill both

    set str [$_owner xml get $path.current]
    if {[string length $str] == 0} {
        set str [$_owner xml get $path.default]
    }
    if {[string length $str] > 0} {
        value $str
    }

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
itcl::body Rappture::ImageEntry::value {args} {
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
        set imh [image create photo -data $newval]

        set oldimh [$itk_component(image) cget -image]
        if {$oldimh != ""} {
            image delete $oldimh
            $itk_component(image) configure -image ""
        }

        $itk_component(image) configure -image $imh
        return $newval

    } elseif {[llength $args] != 0} {
        error "wrong # args: should be \"value ?-check? ?newval?\""
    }

    #
    # Query the value and return.
    #
    set data ""
    set imh [$itk_component(image) cget -image]
    if {$imh != ""} { set data [$imh cget -data] }
    return $data
}

# ----------------------------------------------------------------------
# USAGE: label
#
# Clients use this to query the label associated with this widget.
# Reaches into the XML and pulls out the appropriate label string.
# ----------------------------------------------------------------------
itcl::body Rappture::ImageEntry::label {} {
    set label [$_owner xml get $_path.about.label]
    return [string trim $label]
}

# ----------------------------------------------------------------------
# USAGE: tooltip
#
# Clients use this to query the tooltip associated with this widget.
# Reaches into the XML and pulls out the appropriate description
# string.  Returns the string that should be used with the
# Rappture::Tooltip facility.
# ----------------------------------------------------------------------
itcl::body Rappture::ImageEntry::tooltip {} {
    set str [$_owner xml get $_path.about.description]
    return [string trim $str]
}
