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

itcl::class Rappture::UniRect2d {
    constructor {xmlobj field cname} { # defined below }
    destructor { # defined below }

    public method limits {axis}
    public method blob {}
    public method mesh {}
    public method values {}

    private variable _xmax 0
    private variable _xmin 0
    private variable _xnum 0
    private variable _ymax 0
    private variable _ymin 0
    private variable _ynum 0
    private variable _zv "";		# BLT vector containing the z-values 
}

# ----------------------------------------------------------------------
# Constructor
# ----------------------------------------------------------------------
itcl::body Rappture::UniRect2d::constructor {xmlobj field cname} {
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
    itcl::delete object $mobj

    set _zv [blt::vector create #auto]
    $_zv set [$field get "$cname.values"]
}

# ----------------------------------------------------------------------
# Destructor
# ----------------------------------------------------------------------
itcl::body Rappture::UniRect2d::destructor {} {
    if { $_zv != "" } {
	blt::vector destroy $_zv
    }
}

# ----------------------------------------------------------------------
# method blob 
#	Returns a base64 encoded, gzipped Tcl list that represents the
#	Tcl command and data to recreate the uniform rectangular grid 
#	on the nanovis server.
# ----------------------------------------------------------------------
itcl::body Rappture::UniRect2d::blob {} {
    set data "unirect2d"
    lappend data "xmin" $_xmin "xmax" $_xmax "xnum" $_xnum
    lappend data "ymin" $_ymin "ymax" $_ymax "ynum" $_ynum
    lappend data "xmin" $_xmin "ymin" $_ymin "xmax" $_xmax "ymax" $_ymax
    if { [$_zv length] > 0 } {
	lappend data "zvalues" [$_zv range 0 end]
    }
    return [Rappture::encoding::encode -as zb64 $data]
}

# ----------------------------------------------------------------------
# method mesh 
#	Returns a base64 encoded, gzipped Tcl list that represents the
#	Tcl command and data to recreate the uniform rectangular grid 
#	on the nanovis server.
# ----------------------------------------------------------------------
itcl::body Rappture::UniRect2d::mesh {} {
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
itcl::body Rappture::UniRect2d::values {} {
    if { [$_zv length] > 0 } {
	return [$_zv range 0 end]
    }
    return ""
}

# ----------------------------------------------------------------------
# method limits <axis>
#	Returns a list {min max} representing the limits for the 
#	specified axis.
# ----------------------------------------------------------------------
itcl::body Rappture::UniRect2d::limits {which} {
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
	v - xlin - vlog - z - zlin - zlog {
	    set min [blt::vector expr min($_zv)]
	    set max [blt::vector expr max($_zv)]
	    set axis "zaxis"
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

