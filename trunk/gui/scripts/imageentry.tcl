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

    itk_option define -state state State "normal"

    constructor {owner path args} { # defined below }
    destructor { # defined below }

    public method value {args}

    public method label {}
    public method tooltip {}

    private method _redraw {}

    private variable _owner ""    ;# thing managing this control
    private variable _path ""     ;# path in XML to this image
    private variable _imh ""      ;# image handle for current value
    private variable _resize ""   ;# image for resize operations
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
	::label $itk_interior.image -borderwidth 0
    }
    pack $itk_component(image) -expand yes -fill both
    bind $itk_component(image) <Configure> [itcl::code $this _redraw]

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
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::ImageEntry::destructor {} {
    if {"" != $_imh} { image delete $_imh }
    if {"" != $_resize} { image delete $_resize }
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
	if {[string length $newval] > 0} {
	    set imh [image create photo -data $newval]
	} else {
	    set imh ""
	}

	if {$_imh != ""} {
	    image delete $_imh
	}
	set _imh $imh
	_redraw
	return $newval

    } elseif {[llength $args] != 0} {
	error "wrong # args: should be \"value ?-check? ?newval?\""
    }

    #
    # Query the value and return.
    #
    set data ""
    if {"" != $_imh} { set data [$_imh cget -data] }
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

# ----------------------------------------------------------------------
# USAGE: _redraw
#
# Used internally to update the image displayed in this entry.
# If the <resize> parameter is set, then the image is resized.
# The "auto" value resizes to the current area.  The "width=XX" or
# "height=xx" form resizes to a particular size.  The "none" value
# leaves the image as-is; this is the default.
# ----------------------------------------------------------------------
itcl::body Rappture::ImageEntry::_redraw {} {
    if {"" == $_imh} {
	$itk_component(image) configure -image ""
	return
    }

    set iw [image width $_imh]
    set ih [image height $_imh]
    $itk_component(image) configure -image "" -width $iw -height $ih

    set str [string trim [$_owner xml get $_path.resize]]
    if {"" == $str} {
	set str "none"
    }
    switch -glob -- $str {
	auto {
	    if {$_resize == ""} {
		set _resize [image create photo]
	    }
	    set w [winfo width $itk_component(image)]
	    set h [winfo height $itk_component(image)]
	    if {$w/double($iw) < $h/double($ih)} {
		set h [expr {round($w/double($iw)*$ih)}]
	    } else {
		set w [expr {round($h/double($ih)*$iw)}]
	    }
	    $_resize configure -width $w -height $h
	    blt::winop resample $_imh $_resize
	    $itk_component(image) configure -image $_resize
	}
	width=* - height=* {
	    if {$_resize == ""} {
		set _resize [image create photo]
	    }
	    if {[regexp {^width=([0-9]+)$} $str match size]} {
		set w $size
		set h [expr {round($w*$ih/double($iw))}]
		$_resize configure -width $w -height $h
		blt::winop resample $_imh $_resize
		$itk_component(image) configure -image $_resize \
		    -width $w -height $h
	    } elseif {[regexp {^height=([0-9]+)$} $str match size]} {
		set h $size
		set w [expr {round($h*$iw/double($ih))}]
		$_resize configure -width $w -height $h
		blt::winop resample $_imh $_resize
		$itk_component(image) configure -image $_resize \
		    -width $w -height $h
	    } else {
		$itk_component(image) configure -image $_imh
	    }
	}
	default {
	    $itk_component(image) configure -image $_imh
	}
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -state
# ----------------------------------------------------------------------
itcl::configbody Rappture::ImageEntry::state {
    set valid {normal disabled}
    if {[lsearch -exact $valid $itk_option(-state)] < 0} {
	error "bad value \"$itk_option(-state)\": should be [join $valid {, }]"
    }
    $itk_component(image) configure -state $itk_option(-state)
}
