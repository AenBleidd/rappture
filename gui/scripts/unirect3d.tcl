# -*- mode: tcl; indent-tabs-mode: nil -*- 

# ----------------------------------------------------------------------
#  COMPONENT: unirect3d - represents a uniform rectangular 2-D mesh.
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
    constructor {xmlobj field cname {numComponents 1}} { # defined below }
    destructor { # defined below }

    public method limits {axis}
    public method blob {}
    public method mesh {}
    public method values {}
    public method units { axis }
    public method label { axis }
    public method hints {{keyword ""}} 
    public method order {} {
        return _axisOrder;
    }
    private method GetString { obj path varName }
    private method GetValue { obj path varName }
    private method GetSize { obj path varName }

    private variable _axisOrder  "x y z"
    private variable _xMax       0
    private variable _xMin       0
    private variable _xNum       0;     # Number of points along x-axis
    private variable _yMax       0
    private variable _yMin       0
    private variable _yNum       0;     # Number of points along y-axis
    private variable _zMax       0
    private variable _zMin       0
    private variable _zNum       0;     # Number of points along z-axis
    private variable _compNum    1;     # Number of components in values
    private variable _values     "";    # BLT vector containing the values 
    private variable _hints
}

# ----------------------------------------------------------------------
# Constructor
# ----------------------------------------------------------------------
itcl::body Rappture::Unirect3d::constructor {xmlobj field cname {numComponents 1}} {
    if {![Rappture::library isvalid $xmlobj]} {
        error "bad value \"$xmlobj\": should be Rappture::library"
    }
    set path [$field get $cname.mesh]
    set m [$xmlobj element -as object $path]
    GetValue $m "xaxis.max" _xMax
    GetValue $m "xaxis.min" _xMin
    GetValue $m "yaxis.max" _yMax
    GetValue $m "yaxis.min" _yMin
    GetValue $m "zaxis.max" _zMax
    GetValue $m "zaxis.min" _zMin
    GetSize $m "xaxis.numpoints" _xNum
    GetSize $m "yaxis.numpoints" _yNum
    GetSize $m "zaxis.numpoints" _zNum
    set _compNum $numComponents
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
        order   about.axisorder 
    } {
        set str [$m get $path]
        if {"" != $str} {
            set _hints($key) $str
        }
    }
    foreach {key} { axisorder } {
        set str [$field get $cname.$key]
        if {"" != $str} {
            set _hints($key) $str
        }
    }
    itcl::delete object $m

    set _values [blt::vector create #auto]
    $_values set [$field get "$cname.values"]
    set n [expr $_xNum * $_yNum * $_zNum * $_compNum]
    if { [$_values length] != $n } {
        error "wrong \# of values in \"$cname.values\": expected $n values, got [$_values length]"
    }
    puts stderr "WARNING: The <unirect3d> element is deprecated.  Please use a <mesh> instead."
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
#       Returns a Tcl list that represents the Tcl command and data to
#       recreate the uniform rectangular grid on the nanovis server.
# ----------------------------------------------------------------------
itcl::body Rappture::Unirect3d::blob {} {
    lappend data "unirect3d"
    lappend data "xmin" $_xMin "xmax" $_xMax "xnum" $_xNum
    lappend data "ymin" $_yMin "ymax" $_yMax "ynum" $_yNum
    lappend data "zmin" $_zMin "zmax" $_zMax "znum" $_zNum
    lappend data "axisorder" $_axisOrder
    if { [$_values length] > 0 } {
        lappend data "values" [$_values range 0 end]
    }
    return $data
}

# ----------------------------------------------------------------------
# method mesh 
#       Returns a Tcl list that represents the points of the uniform
#       grid.  Each point has x,y and z values in the list.
# ----------------------------------------------------------------------
itcl::body Rappture::Unirect3d::mesh {} {
    set dx [expr {($_xMax - $_xMin) / double($_xNum - 1)}]
    set dy [expr {($_yMax - $_yMin) / double($_yNum - 1)}]
    set dz [expr {($_zMax - $_zMin) / double($_zNum - 1)}]
    foreach {a b c} $_axisOrder break
    for { set i 0 } { $i < [set _${a}Num] } { incr i } {
        set v1 [expr {[set _${a}Min] + (double($i) * [set d${a}])}]
        for { set j 0 } { $j < [set _${b}Num] } { incr j } {
            set v2 [expr {[set _${b}Min] + (double($i) * [set d${b}])}]
            for { set k 0 } { $k < [set _${c}Num] } { incr k } {
                set v3 [expr {[set _${c}Min] + (double($i) * [set d${c}])}]
                lappend data $v1 $v2 $v3
            }
        }
    }
    return $data
}

# ----------------------------------------------------------------------
# method values
#       Returns a BLT vector that represents the field values
# ----------------------------------------------------------------------
itcl::body Rappture::Unirect3d::values {} {
    return $_values
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
        v - vlin - vlog {
            if { [$_values length] > 0 } {
               set min [blt::vector expr min($_values)]
               set max [blt::vector expr max($_values)]
            } else {
                set min 0.0
                set max 1.0
            }
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
    if { [info exists _hints({$axis}units)] } {
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
    if { [info exists _hints({$axis}label)] } {
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
itcl::body Rappture::Unirect3d::hints {{keyword ""}} {
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
