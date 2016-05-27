# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: MapEntry - widget for viewing map as input control
#
# ======================================================================
#  AUTHOR:  Leif Delgass, Purdue University
#  Copyright (c) 2016  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

itcl::class Rappture::MapEntry {
    inherit itk::Widget

    itk_option define -state state State "normal"

    constructor {owner path args} {
        # defined below
    }
    public method label {}
    public method tooltip {}
    public method value {args}

    private variable _owner ""      ;# thing managing this control
    private variable _path ""       ;# path in XML to this input
    private variable _xmlobj ""
    private variable _mapviewer ""  ;# MapViewer widget
}

itk::usual MapEntry {
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::MapEntry::constructor {owner path args} {
    if {[catch {$owner isa Rappture::ControlOwner} valid] != 0 || !$valid} {
        error "bad object \"$owner\": should be Rappture::ControlOwner"
    }
    set _owner $owner
    set _path $path
    set _xmlobj [$_owner xml object]

    itk_component add map {
        Rappture::MapViewer $itk_interior.map
    }
    pack $itk_component(map) -expand yes -fill both
    set _mapviewer $itk_component(map)

    eval itk_initialize $args

    value $_xmlobj
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
itcl::body Rappture::MapEntry::value {args} {
    set onlycheck 0
    set i [lsearch -exact $args -check]
    if {$i >= 0} {
        set onlycheck 1
        set args [lreplace $args $i $i]
    }

    if {[llength $args] == 1} {
        set newval [lindex $args 0]

        if {!$onlycheck && $_xmlobj != "" && $_xmlobj != $newval} {
            # delete any existing object
            itcl::delete object $_xmlobj
            set _xmlobj ""
        }
        if {$newval != ""} {
            if {![Rappture::library isvalid $newval]} {
                error "bad value \"$newval\": should be Rappture::Library"
            }
            if {$onlycheck} {
                return
            }
            set _xmlobj $newval
        }
        set dataobjs [$_mapviewer get]
        $_mapviewer configure -map [Rappture::Map ::#auto $_xmlobj $_path]
        foreach dataobj $dataobjs {
            itcl::delete object $dataobj
        }
        event generate $itk_component(hull) <<Value>>
    } elseif {[llength $args] != 0} {
        error "wrong # args: should be \"value ?-check? ?newval?\""
    }
    return ""
}

# ----------------------------------------------------------------------
# USAGE: label
#
# Clients use this to query the label associated with this widget.
# Reaches into the XML and pulls out the appropriate label string.
# ----------------------------------------------------------------------
itcl::body Rappture::MapEntry::label {} {
    set label [string trim [$_owner xml get $_path.about.label]]
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
itcl::body Rappture::MapEntry::tooltip {} {
    set str [string trim [$_owner xml get $_path.about.description]]
    return $str
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -state
# ----------------------------------------------------------------------
itcl::configbody Rappture::MapEntry::state {
    set valid {normal disabled}
    if {[lsearch -exact $valid $itk_option(-state)] < 0} {
        error "bad value \"$itk_option(-state)\": should be [join $valid {, }]"
    }
    #$_mapviewer configure -state $itk_option(-state)
}
