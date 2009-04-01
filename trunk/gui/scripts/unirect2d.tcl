
# ----------------------------------------------------------------------
#  COMPONENT: unirect2d - represents a uniform rectangular 2-D mesh.
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

itcl::class Rappture::Unirect2d {
    constructor {xmlobj field cname} { # defined below }
    destructor { # defined below }

    public method limits {axis}
    public method blob {}
    public method mesh {}
    public method values {}
    public method hints {{keyword ""}} 

    private variable _xmax 0
    private variable _xmin 0
    private variable _xnum 0
    private variable _ymax 0
    private variable _ymin 0
    private variable _ynum 0
    private variable _values "";	# BLT vector containing the z-values 
    private variable _hints
}

# ----------------------------------------------------------------------
# Constructor
# ----------------------------------------------------------------------
itcl::body Rappture::Unirect2d::constructor {xmlobj field cname} {
    if {![Rappture::library isvalid $xmlobj]} {
	error "bad value \"$xmlobj\": should be Rappture::library"
    }
    set path [$field get $cname.mesh]

    set mobj [$xmlobj element -as object $path]
    set _xmin [$mobj get "xaxis.min"]
    set _xmax [$mobj get "xaxis.max"]
    set _xnum [$mobj get "xaxis.numpoints"]
    set _ymin [$mobj get "yaxis.min"]
    set _ymax [$mobj get "yaxis.max"]
    set _ynum [$mobj get "yaxis.numpoints"]
    
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
	xmin    xaxis.min
	xmax    xaxis.max
	ylabel  yaxis.label
	ydesc   yaxis.description
	yunits  yaxis.units
	yscale  yaxis.scale
	ymin    yaxis.min
	ymax    yaxis.max
    } {
	set str [$mobj get $path]
	if {"" != $str} {
	    set _hints($key) $str
	}
    }
    foreach {key} { extents axisorder } {
	set str [$field get $cname.$key]
	if {"" != $str} {
	    set _hints($key) $str
	}
    }
    itcl::delete object $mobj
    
    set _values [blt::vector create \#auto]
    set values [$field get "$cname.values"]
    if { $values == "" } {
	set values [$field get "$cname.zvalues"]
    }
    $_values set $values
}

# ----------------------------------------------------------------------
# Destructor
# ----------------------------------------------------------------------
itcl::body Rappture::Unirect2d::destructor {} {
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
itcl::body Rappture::Unirect2d::blob {} {
    set data "unirect2d"
    lappend data "xmin" $_xmin "xmax" $_xmax "xnum" $_xnum
    lappend data "ymin" $_ymin "ymax" $_ymax "ynum" $_ynum
    lappend data "xmin" $_xmin "ymin" $_ymin "xmax" $_xmax "ymax" $_ymax
    foreach key { axisorder extents xunits yunits units } {
	set hint [hints $key]
	if { $hint != "" } {
	    lappend data $key $hint
	}
    }
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
itcl::body Rappture::Unirect2d::mesh {} {
    set dx [expr {($_xmax - $_xmin) / double($_xnum)}]
    set dy [expr {($_ymax - $_ymin) / double($_ynum)}]
    for { set i 0 } { $i < $_xnum } { incr i } {
	set x [expr {$_xmin + (double($i) * $dx)}]
	for { set j 0 } { $j < $_ynum } { incr j } {
	    set y [expr {$_ymin + (double($i) * $dy)}]
	    lappend data $x $y
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
itcl::body Rappture::Unirect2d::values {} {
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
itcl::body Rappture::Unirect2d::limits {which} {
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
	v - vlin - vlog - z - zlin - zlog {
	    if { [$_values length] > 0 } {
	       set min [blt::vector expr min($_values)]
	       set max [blt::vector expr max($_values)]
	    } else {
		set min 0.0
		set max 1.0
	    }
	    set axis "zaxis"
	}
	default {
	    error "unknown axis description \"$which\""
	}
    }
#     set val [$_field get $axis.min]
#     if {"" != $val && "" != $min} {
#         if {$val > $min} {
#             # tool specified this min -- don't go any lower
#             set min $val
#         }
#     }
#     set val [$_field get $axis.max]
#     if {"" != $val && "" != $max} {
#         if {$val < $max} {
#             # tool specified this max -- don't go any higher
#             set max $val
#         }
#     }

    return [list $min $max]
}


# ----------------------------------------------------------------------
# USAGE: hints ?<keyword>?
#
# Returns a list of key/value pairs for various hints about plotting
# this curve.  If a particular <keyword> is specified, then it returns
# the hint for that <keyword>, if it exists.
# ----------------------------------------------------------------------
itcl::body Rappture::Unirect2d::hints { {keyword ""} } {
    if {[info exists _hints(xlabel)] && "" != $_hints(xlabel)
	&& [info exists _hints(xunits)] && "" != $_hints(xunits)} {
	set _hints(xlabel) "$_hints(xlabel) ($_hints(xunits))"
    }
    if {[info exists _hints(ylabel)] && "" != $_hints(ylabel)
	&& [info exists _hints(yunits)] && "" != $_hints(yunits)} {
	set _hints(ylabel) "$_hints(ylabel) ($_hints(yunits))"
    }
    
    if {[info exists _hints(group)] && [info exists _hints(label)]} {
	# pop-up help for each curve
	set _hints(tooltip) $_hints(label)
    }
    if {$keyword != ""} {
	if {[info exists _hints($keyword)]} {
	    return $_hints($keyword)
	}
	return ""
    }
    return [array get _hints]
}
