
# ----------------------------------------------------------------------
#  COMPONENT: unirect3d - represents a uniform rectangular 2-D mesh.
#
#  This object represents one field in an XML description of a device.
#  It simplifies the process of extracting data vectors that represent
#  the field.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itcl
package require BLT

namespace eval Rappture { # forward declaration }

itcl::class Rappture::Unirect3d {
    constructor {xmlobj field cname} { # defined below }
    destructor { # defined below }

    public method limits {axis}
    public method blob {}
    public method mesh {}
    public method values {}
    public method hints {{keyword ""}} 
    public method order {} {
	return _order;
    }
    public method dimensions {} {
	return _dim;
    }
    private variable _order	 "x y z"
    private variable _dimensions 1
    private variable _xmax	 0
    private variable _xmin	 0
    private variable _xnum	 0
    private variable _ymax	 0
    private variable _ymin	 0
    private variable _ynum	 0
    private variable _zmax	 0
    private variable _zmin	 0
    private variable _znum	 0
    private variable _values	 ""; # BLT vector containing the z-values 
    private variable _hints
}

# ----------------------------------------------------------------------
# Constructor
# ----------------------------------------------------------------------
itcl::body Rappture::Unirect3d::constructor {xmlobj field cname} {
    if {![Rappture::library isvalid $xmlobj]} {
	error "bad value \"$xmlobj\": should be Rappture::library"
    }
    set path [$field get $cname.mesh]

    set mobj [$xmlobj element -as object $path]
    set order [$mobj get "axisorder"]
    if { $order != "" } {
	set _order $order
    }
    set dim [$mobj get "dimensions"]
    if { $dim != "" } {
	set _dimensions $dim
    }
    set _xmin [$mobj get "xaxis.min"]
    set _xmax [$mobj get "xaxis.max"]
    set _xnum [$mobj get "xaxis.numpoints"]
    set _ymin [$mobj get "yaxis.min"]
    set _ymax [$mobj get "yaxis.max"]
    set _ynum [$mobj get "yaxis.numpoints"]
    set _zmin [$mobj get "zaxis.min"]
    set _zmax [$mobj get "zaxis.max"]
    set _znum [$mobj get "zaxis.numpoints"]
    itcl::delete object $mobj

    set _values [blt::vector create #auto]
    $_values set [$field get "$cname.values"]
}

# ----------------------------------------------------------------------
# Destructor
# ----------------------------------------------------------------------
itcl::body Rappture::Unirect3d::destructor {} {
    if { $_values != "" } {
	blt::vector destroy $_values
    }
}

# ----------------------------------------------------------------------
# method blob 
#	Returns a base64 encoded, gzipped Tcl list that represents the
#	Tcl command and data to recreate the uniform rectangular grid 
#	on the nanovis server.
# ----------------------------------------------------------------------
itcl::body Rappture::Unirect3d::blob {} {
    set data "unirect3d"
    lappend data "xmin" $_xmin "xmax" $_xmax "xnum" $_xnum
    lappend data "ymin" $_ymin "ymax" $_ymax "ynum" $_ynum
    lappend data "zmin" $_zmin "zmax" $_zmax "znum" $_znum
    lappend data "axisorder" $_axisorder
    lappend data "dimensions" $_dim
    if { [$_values length] > 0 } {
	lappend data "values" [$_values range 0 end]
    }
    return [Rappture::encoding::encode -as zb64 $data]
}

# ----------------------------------------------------------------------
# method mesh 
#	Returns a base64 encoded, gzipped Tcl list that represents the
#	Tcl command and data to recreate the uniform rectangular grid 
#	on the nanovis server.
# ----------------------------------------------------------------------
itcl::body Rappture::Unirect3d::mesh {} {
    set dx [expr {($_xmax - $_xmin) / double($_xnum)}]
    set dy [expr {($_ymax - $_ymin) / double($_ynum)}]
    set dz [expr {($_zmax - $_zmin) / double($_znum)}]
    foreach {a b c} $_axisorder break
    for { set i 0 } { $i < [set _${a}num] } { incr i } {
	set v1 [expr {[set _${a}min] + (double($i) * [set d${a}])}]
	for { set j 0 } { $j < [set _${b}num] } { incr j } {
	    set v2 [expr {[set _${b}min] + (double($i) * [set d${b}])}]
	    for { set k 0 } { $k < [set _{$c}num] } { incr k } {
		set v3 [expr {[set _${c}min] + (double($i) * [set d${c}])}]
		lappend data $v1 $v2 $v3
	    }
	}
    }
    return $data
}

# ----------------------------------------------------------------------
# method values 
#	Returns a base64 encoded, gzipped Tcl list that represents the
#	Tcl command and data to recreate the uniform rectangular grid 
#	on the nanovis server.
# ----------------------------------------------------------------------
itcl::body Rappture::Unirect3d::values {} {
    if { [$_values length] > 0 } {
	return [$_values range 0 end]
    }
    return ""
}

# ----------------------------------------------------------------------
# method limits <axis>
#	Returns a list {min max} representing the limits for the 
#	specified axis.
# ----------------------------------------------------------------------
itcl::body Rappture::Unirect3d::limits {which} {
    set min ""
    set max ""

    switch -- $which {
	x - xlin - xlog {
	    set min $_xmin
	    set max $_xmax
	    set axis "xaxis"
	}
	y - ylin - ylog {
	    set min $_ymin
	    set max $_ymax
	    set axis "yaxis"
	}
	z - zlin - zlog {
	    set min $_zmin
	    set max $_zmax
	    set axis "zaxis"
	}
	v - vlin - vlog {
	    if { [$_values length] > 0 } {
	       set min [blt::vector expr min($_values)]
	       set max [blt::vector expr max($_values)]
	    } else {
		set min 0.0
		set max 1.0
	    }
	    set axis "vaxis"
	}
	default {
	    error "unknown axis description \"$which\""
	}
    }
    return [list $min $max]
}


# ----------------------------------------------------------------------
# USAGE: hints ?<keyword>?
#
# Returns a list of key/value pairs for various hints about plotting
# this curve.  If a particular <keyword> is specified, then it returns
# the hint for that <keyword>, if it exists.
# ----------------------------------------------------------------------
itcl::body Rappture::Unirect3d::hints {{keyword ""}} {
    if {![info exists _hints]} {
	foreach {key path} {
	    group   about.group
	    label   about.label
	    color   about.color
	    style   about.style
	    type    about.type
	    xlabel  xaxis.label
	    xdesc   xaxis.description
	    xunits  xaxis.units
	    xscale  xaxis.scale
	    ylabel  yaxis.label
	    ydesc   yaxis.description
	    yunits  yaxis.units
	    yscale  yaxis.scale
	    zlabel  zaxis.label
	    zdesc   zaxis.description
	    zunits  zaxis.units
	    zscale  zaxis.scale
	    dim     about.dimension 
	    order   about.axisorder 
	} {
	    set str [$_curve get $path]
	    if {"" != $str} {
		set _hints($key) $str
	    }
	}

	if {[info exists _hints(xlabel)] && "" != $_hints(xlabel)
	      && [info exists _hints(xunits)] && "" != $_hints(xunits)} {
	    set _hints(xlabel) "$_hints(xlabel) ($_hints(xunits))"
	}
	if {[info exists _hints(ylabel)] && "" != $_hints(ylabel)
	      && [info exists _hints(yunits)] && "" != $_hints(yunits)} {
	    set _hints(ylabel) "$_hints(ylabel) ($_hints(yunits))"
	}
	if {[info exists _hints(zlabel)] && "" != $_hints(zlabel)
	      && [info exists _hints(zunits)] && "" != $_hints(zunits)} {
	    set _hints(ylabel) "$_hints(zlabel) ($_hints(zunits))"
	}
	if {[info exists _hints(group)] && [info exists _hints(label)]} {
	    # pop-up help for each curve
	    set _hints(tooltip) $_hints(label)
	}
    }
    if {$keyword != ""} {
	if {[info exists _hints($keyword)]} {
	    return $_hints($keyword)
	}
	return ""
    }
    return [array get _hints]
}
