# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: unirect3d - represents a uniform rectangular 3-D mesh.
#
#  This object represents one field in an XML description of a device.
#  It simplifies the process of extracting data vectors that represent
#  the field.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itcl
package require BLT

namespace eval Rappture { # forward declaration }

itcl::class Rappture::Unirect3d {
    constructor {xmlobj path} {
        # defined below
    }
    destructor {
        # defined below
    }
    public proc fetch {xmlobj path}
    public proc release {obj}

    public method dimensions {} {
        return 3
    }
    public method hints {{keyword ""}}
    public method isvalid {} {
        return $_isValid
    }
    public method label { axis }
    public method limits {axis}
    public method numpoints {} {
        return $_numPoints
    }
    public method units { axis }
    public method vtkdata {{what -partial}} {}

    private method GetString { obj path varName }
    private method GetValue { obj path varName }
    private method GetSize { obj path varName }

    private variable _xMax       0
    private variable _xMin       0
    private variable _xNum       0;     # Number of points along x-axis
    private variable _yMax       0
    private variable _yMin       0
    private variable _yNum       0;     # Number of points along y-axis
    private variable _zMax       0
    private variable _zMin       0
    private variable _zNum       0;     # Number of points along z-axis
    private variable _hints
    private variable _vtkdata    ""
    private variable _numPoints  0
    private variable _isValid    0;     # Indicates if the data is valid.

    private common _xp2obj       ;      # used for fetch/release ref counting
    private common _obj2ref      ;      # used for fetch/release ref counting
}

#
# fetch <xmlobj> <path>
#
#    Clients use this instead of a constructor to fetch the Mesh for a
#    particular <path> in the <xmlobj>.  When the client is done with the mesh,
#    he calls "release" to decrement the reference count.  When the mesh is no
#    longer needed, it is cleaned up automatically.
#
itcl::body Rappture::Unirect3d::fetch {xmlobj path} {
    set handle "$xmlobj|$path"
    if {[info exists _xp2obj($handle)]} {
        set obj $_xp2obj($handle)
        incr _obj2ref($obj)
        return $obj
    }
    set obj [Rappture::Unirect3d ::#auto $xmlobj $path]
    set _xp2obj($handle) $obj
    set _obj2ref($obj) 1
    return $obj
}

# ----------------------------------------------------------------------
# USAGE: Rappture::Unirect3d::release <obj>
#
# Clients call this when they're no longer using a Mesh fetched
# previously by the "fetch" proc.  This decrements the reference
# count for the mesh and destroys the object when it is no longer
# in use.
# ----------------------------------------------------------------------
itcl::body Rappture::Unirect3d::release { obj } {
    if { ![info exists _obj2ref($obj)] } {
        error "can't find reference count for $obj"
    }
    incr _obj2ref($obj) -1
    if {$_obj2ref($obj) <= 0} {
        unset _obj2ref($obj)
        foreach handle [array names _xp2obj] {
            if {$_xp2obj($handle) == $obj} {
                unset _xp2obj($handle)
            }
        }
        itcl::delete object $obj
    }
}

# ----------------------------------------------------------------------
# Constructor
# ----------------------------------------------------------------------
itcl::body Rappture::Unirect3d::constructor {xmlobj path} {
    if {![Rappture::library isvalid $xmlobj]} {
        error "bad value \"$xmlobj\": should be Rappture::library"
    }
    set m [$xmlobj element -as object $path]
    GetValue $m "xaxis.min" _xMin
    GetValue $m "xaxis.max" _xMax
    GetValue $m "yaxis.min" _yMin
    GetValue $m "yaxis.max" _yMax
    GetValue $m "zaxis.min" _zMin
    GetValue $m "zaxis.max" _zMax
    GetSize $m "xaxis.numpoints" _xNum
    GetSize $m "yaxis.numpoints" _yNum
    GetSize $m "zaxis.numpoints" _zNum
    foreach {key path} {
        label   about.label
        color   about.color
        style   about.style
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
        zlabel  zaxis.label
        zdesc   zaxis.description
        zunits  zaxis.units
        zscale  zaxis.scale
        zmin    zaxis.min
        zmax    zaxis.max
    } {
        set str [$m get $path]
        if {"" != $str} {
            set _hints($key) $str
        }
    }
    itcl::delete object $m
    set _numPoints [expr $_xNum * $_yNum * $_zNum]
    if { $_numPoints == 0 } {
        set _vtkdata ""
        return
    }
    append out "DATASET STRUCTURED_POINTS\n"
    append out "DIMENSIONS $_xNum $_yNum $_zNum\n"
    append out "ORIGIN $_xMin $_yMin $_zMin\n"
    if { $_xNum > 1 } {
        set xSpace [expr (double($_xMax) - double($_xMin))/double($_xNum - 1)]
    } else {
        set xSpace 0.0
    }
    if { $_yNum > 1 } {
        set ySpace [expr (double($_yMax) - double($_yMin))/double($_yNum - 1)]
    } else {
        set ySpace 0.0
    }
    if { $_zNum > 1 } {
        set zSpace [expr (double($_zMax) - double($_zMin))/double($_zNum - 1)]
    } else {
        set zSpace 0.0
    }
    append out "SPACING $xSpace $ySpace $zSpace\n"
    set _vtkdata $out
    set _isValid 1
    puts stderr "WARNING: The <unirect3d> element is deprecated.  Please use a <mesh> instead."
}

# ----------------------------------------------------------------------
# Destructor
# ----------------------------------------------------------------------
itcl::body Rappture::Unirect3d::destructor {} {
    # empty
}

# ----------------------------------------------------------------------
# method limits <axis>
#       Returns a list {min max} representing the limits for the
#       specified axis.
# ----------------------------------------------------------------------
itcl::body Rappture::Unirect3d::limits {which} {
    set min ""
    set max ""

    switch -- $which {
        x - xlin - xlog {
            set min $_xMin
            set max $_xMax
        }
        y - ylin - ylog {
            set min $_yMin
            set max $_yMax
        }
        z - zlin - zlog {
            set min $_zMin
            set max $_zMax
        }
        default {
            error "unknown axis description \"$which\""
        }
    }
    return [list $min $max]
}

#
# units --
#
#       Returns the units of the given axis.
#
itcl::body Rappture::Unirect3d::units { axis } {
    if { [info exists _hints(${axis}units)] } {
        return $_hints(${axis}units)
    }
    return ""
}

#
# label --
#
#       Returns the label of the given axis.
#
itcl::body Rappture::Unirect3d::label { axis } {
    if { [info exists _hints(${axis}label)] } {
        return $_hints(${axis}label)
    }
    return ""
}

# ----------------------------------------------------------------------
# USAGE: hints ?<keyword>?
#
# Returns a list of key/value pairs for various hints about plotting
# this curve.  If a particular <keyword> is specified, then it returns
# the hint for that <keyword>, if it exists.
# ----------------------------------------------------------------------
itcl::body Rappture::Unirect3d::hints { {keyword ""} } {
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
    if {$keyword != ""} {
        if {[info exists _hints($keyword)]} {
            return $_hints($keyword)
        }
        return ""
    }
    return [array get _hints]
}

itcl::body Rappture::Unirect3d::vtkdata {{what -partial}} {
    if {$what == "-full"} {
        append out "# vtk DataFile Version 3.0\n"
        append out "[hints label]\n"
        append out "ASCII\n"
        append out $_vtkdata
        return $out
    } else {
        return $_vtkdata
    }
}

itcl::body Rappture::Unirect3d::GetSize { obj path varName } {
    set string [$obj get $path]
    if { [scan $string "%d" value] != 1 || $value < 0 } {
        puts stderr "can't get size \"$string\" of \"$path\""
        return
    }
    upvar $varName size
    set size $value
}

itcl::body Rappture::Unirect3d::GetValue { obj path varName } {
    set string [$obj get $path]
    if { [scan $string "%g" value] != 1 } {
        puts stderr "can't get value \"$string\" of \"$path\""
        return
    }
    upvar $varName number
    set number $value
}

itcl::body Rappture::Unirect3d::GetString { obj path varName } {
    set string [$obj get $path]
    if { $string == "" } {
        puts stderr "can't get string \"$string\" of \"$path\""
        return
    }
    upvar $varName str
    set str $string
}
