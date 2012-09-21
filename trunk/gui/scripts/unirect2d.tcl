
# ----------------------------------------------------------------------
#  COMPONENT: unirect2d - represents a uniform rectangular 2-D mesh.
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

itcl::class Rappture::Unirect2d {
    constructor {xmlobj field cname {extents 1}} { # defined below }
    destructor { # defined below }

    public method limits {axis}
    public method blob {}
    public method mesh.old {}
    public method mesh {}
    public method values {}
    public method hints {{keyword ""}} 
    private method GetString { obj path varName }
    private method GetValue { obj path varName }
    private method GetSize { obj path varName }

    private variable _axisOrder "x y"
    private variable _xMax      0
    private variable _xMin      0
    private variable _xNum      0
    private variable _yMax      0
    private variable _yMin      0
    private variable _yNum      0
    private variable _compNum   1
    private variable _values    "";     # BLT vector containing the z-values 
    private variable _hints
}

# ----------------------------------------------------------------------
# Constructor
# ----------------------------------------------------------------------
itcl::body Rappture::Unirect2d::constructor {xmlobj field cname {extents 1}} {
    if {![Rappture::library isvalid $xmlobj]} {
        error "bad value \"$xmlobj\": should be Rappture::library"
    }
    set path [$field get $cname.mesh]

    set m [$xmlobj element -as object $path]
    GetValue $m "xaxis.min" _xMin
    GetValue $m "xaxis.max" _xMax
    GetSize $m "xaxis.numpoints" _xNum
    GetValue $m "yaxis.min" _yMin
    GetValue $m "yaxis.max" _yMax
    GetSize $m "yaxis.numpoints" _yNum
    set _compNum $extents
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
        type    about.type
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
    
    set _values [blt::vector create \#auto]
    set values [$field get "$cname.values"]
    if { $values == "" } {
        set values [$field get "$cname.zvalues"]
    }
    $_values set $values
    set n [expr $_xNum * $_yNum * $_compNum]
    if { [$_values length] != $n } {
        error "wrong \# of values in \"$cname.values\": expected $n values, got [$_values length]"
    }
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
#       Returns a base64 encoded, gzipped Tcl list that represents the
#       Tcl command and data to recreate the uniform rectangular grid 
#       on the nanovis server.
# ----------------------------------------------------------------------
itcl::body Rappture::Unirect2d::blob {} {
    set data "unirect2d"
    lappend data "xmin" $_xMin "xmax" $_xMax "xnum" $_xNum
    lappend data "ymin" $_yMin "ymax" $_yMax "ynum" $_yNum
    lappend data "xmin" $_xMin "ymin" $_yMin "xmax" $_xMax "ymax" $_yMax
    foreach key { axisorder xunits yunits units } {
        set hint [hints $key]
        if { $hint != "" } {
            lappend data $key $hint
        }
    }
    if { [$_values length] > 0 } {
        lappend data "values" [$_values range 0 end]
    }
    return [Rappture::encoding::encode -as zb64 "$data"]
}

# ----------------------------------------------------------------------
# method mesh.old 
#       Returns a base64 encoded, gzipped Tcl list that represents the
#       Tcl command and data to recreate the uniform rectangular grid 
#       on the nanovis server.
# ----------------------------------------------------------------------
itcl::body Rappture::Unirect2d::mesh.old {} {
    set dx [expr {($_xMax - $_xMin) / double($_xNum)}]
    set dy [expr {($_yMax - $_yMin) / double($_yNum)}]
    foreach {a b} $_axisOrder break
    for { set i 0 } { $i < [set _${a}Num] } { incr i } {
        set x [expr {$_xMin + (double($i) * $dx)}]
        for { set j 0 } { $j < [set _${b}Num] } { incr j } {
            set y [expr {$_yMin + (double($i) * $dy)}]
            lappend data $x $y
        }
    }
    return $data
}

# ----------------------------------------------------------------------
# method mesh
#       Returns a base64 encoded, gzipped Tcl list that represents the
#       Tcl command and data to recreate the uniform rectangular grid 
#       on the nanovis server.
# ----------------------------------------------------------------------
itcl::body Rappture::Unirect2d::mesh {} {
    lappend out $_xMin $_xMax $_xNum $_yMin $_yMax $_yNum
    return $out
}

# ----------------------------------------------------------------------
# method values 
#       Returns a base64 encoded, gzipped Tcl list that represents the
#       Tcl command and data to recreate the uniform rectangular grid 
#       on the nanovis server.
# ----------------------------------------------------------------------
itcl::body Rappture::Unirect2d::values {} {
    if { [$_values length] > 0 } {
        return [$_values range 0 end]
    }
    return ""
}

# ----------------------------------------------------------------------
# method limits <axis>
#       Returns a list {min max} representing the limits for the 
#       specified axis.
# ----------------------------------------------------------------------
itcl::body Rappture::Unirect2d::limits {which} {
    set min ""
    set max ""

    switch -- $which {
        x - xlin - xlog {
            set min $_xMin
            set max $_xMax
            set axis "xaxis"
        }
        y - ylin - ylog {
            set min $_yMin
            set max $_yMax
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


itcl::body Rappture::Unirect2d::GetSize { obj path varName } {
    set string [$obj get $path]
    if { [scan $string "%d" value] != 1 || $value < 0 } {
        puts stderr "can't get size \"$string\" of \"$path\""
        return
    }
    upvar $varName size
    set size $value
}

itcl::body Rappture::Unirect2d::GetValue { obj path varName } {
    set string [$obj get $path]
    if { [scan $string "%g" value] != 1 } {
        return
    }
    upvar $varName number
    set number $value
}

itcl::body Rappture::Unirect2d::GetString { obj path varName } {
    set string [$obj get $path]
    if { $string == "" } {
        puts stderr "can't get string \"$string\" of \"$path\""
        return
    }
    upvar $varName str
    set str $string
}
