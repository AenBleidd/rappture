
# ----------------------------------------------------------------------
#  COMPONENT: drawing3d - represents a vtk drawing.
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
package require vtk

namespace eval Rappture { 
    # forward declaration 
}

itcl::class Rappture::Drawing3d {
    constructor {xmlobj path} { 
        # defined below 
    }
    destructor { 
        # defined below 
    }
    public method limits {axis}
    public method style { elem }
    public method values { elem }
    public method data { elem }
    public method hints {{keyword ""}} 
    public method components { args } 

    private variable _drawing3d
    private variable _xmlobj 
    private variable _actors 
    private variable _styles 
    private variable _data 
    private variable _hints
    private variable _units
    private variable _limits
}

# ----------------------------------------------------------------------
# Constructor
# ----------------------------------------------------------------------
itcl::body Rappture::Drawing3d::constructor {xmlobj path} {
    if {![Rappture::library isvalid $xmlobj]} {
        error "bad value \"$xmlobj\": should be Rappture::library"
    }
    set _xmlobj $xmlobj
    set _drawing3d [$xmlobj element -as object $path]
    set _units [$_drawing3d get units]

    set xunits [$xmlobj get units]
    if {"" == $xunits || "arbitrary" == $xunits} {
        set xunits "um"
    }
    array set _limits {
        xMin 0 
        xMax 0 
        yMin 0 
        yMax 0 
        zMin 0 
        zMax 0 
    }
    # determine the overall size of the device
    foreach elem [$_xmlobj children $path] {
        switch -glob -- $elem {
            polygon* {
                set data [$_xmlobj get $path.$elem.vtk]
                set arr [vtkCharArray $this-xvtkCharArray]
                $arr SetArray $data [string length $data] 1
                set reader [vtkPolyDataReader $this-xvtkPolyDataReader]
                $reader SetInputArray $arr
                $reader ReadFromInputStringOn
                set mapper [vtkPolyDataMapper $this-xvtkPolyDataMapper]
                $mapper SetInput [$reader GetOutput]
                set actor [vtkActor $this-xvthActor]
                $actor SetMapper $mapper
                set _actors($elem) $actor
                set _limits($elem) [$actor GetBounds]
                set _styles($elem) [$_xmlobj get $path.$elem.style]
                set _data($elem) $mapper
            }
        }
    }
    foreach {key path} {
        group   about.group
        label   about.label
        color   about.color
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
        zlabel  zaxis.label
        zdesc   zaxis.description
        zunits  zaxis.units
        zscale  zaxis.scale
        zmin    zaxis.min
        zmax    zaxis.max
    } {
        set str [$_drawing3d get $path]
        if {"" != $str} {
            set _hints($key) $str
        }
    }
    foreach {key} { axisorder } {
        set str [$_drawing3d get $key]
        if {"" != $str} {
            set _hints($key) $str
        }
    }
}

# ----------------------------------------------------------------------
# Destructor
# ----------------------------------------------------------------------
itcl::body Rappture::Drawing3d::destructor {} {
    foreach key [array names _actors] {
        set actor _actors($key)
    }
}

# ----------------------------------------------------------------------
# method style 
#	Returns a base64 encoded, gzipped Tcl list that represents the
#	Tcl command and data to recreate the uniform rectangular grid 
#	on the nanovis server.
# ----------------------------------------------------------------------
itcl::body Rappture::Drawing3d::style { elem } {
    if { [info exists _styles($elem)] } {
        return $_styles($elem)
    } 
    return ""
}

# ----------------------------------------------------------------------
# method data 
#	Returns a base64 encoded, gzipped Tcl list that represents the
#	Tcl command and data to recreate the uniform rectangular grid 
#	on the nanovis server.
# ----------------------------------------------------------------------
itcl::body Rappture::Drawing3d::data { elem } {
    if { [info exists _data($elem)] } {
        return $_data($elem)
    } 
    return ""
}

# ----------------------------------------------------------------------
# method values 
#	Returns a base64 encoded, gzipped Tcl list that represents the
#	Tcl command and data to recreate the uniform rectangular grid 
#	on the nanovis server.
# ----------------------------------------------------------------------
itcl::body Rappture::Drawing3d::values { elem } {
    if { [info exists _actors($elem)] } {
        return $_actors($elem)
    } 
    return ""
}

itcl::body Rappture::Drawing3d::components { args } {
    return [array names _actors] 
}

# ----------------------------------------------------------------------
# method limits <axis>
#	Returns a list {min max} representing the limits for the 
#	specified axis.
# ----------------------------------------------------------------------
itcl::body Rappture::Drawing3d::limits {which} {
    set min ""
    set max ""
    foreach key [array names _actors] {
        set actor $_actors($key)
        foreach key { xMin xMax yMin yMax zMin zMax} value [$actor GetBounds] {
            set _limits($key) $value
        }
        break
    }    
    
    foreach key [array names _actors] {
        set actor $_actors($key)
        foreach { xMin xMax yMin yMax zMin zMax} [$actor GetBounds] break
        if { $xMin < $_limits(xMin) } {
            set _limits(xMin) $xMin
        } 
        if { $xMax > $_limits(xMax) } {
            set _limits(xMax) $xMax
        } 
        if { $yMin < $_limits(yMin) } {
            set _limits(yMin) $yMin
        } 
        if { $yMax > $_limits(yMax) } {
            set _limits(yMax) $yMax
        } 
        if { $zMin < $_limits(zMin) } {
            set _limits(zMin) $zMin
        } 
        if { $zMax > $_limits(zMax) } {
            set _limits(zMax) $zMax
        } 
    }
    switch -- $which {
        x {
            set min $_limits(xMin)
            set max $_limits(xMax)
            set axis "xaxis"
        }
        y {
            set min $_limits(yMin)
            set max $_limits(yMax)
            set axis "yaxis"
        }
        v - z {
            set min $_limits(zMin)
            set max $_limits(zMax)
            set axis "zaxis"
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
itcl::body Rappture::Drawing3d::hints { {keyword ""} } {
    if 0 {
    if {[info exists _hints(xlabel)] && "" != $_hints(xlabel)
        && [info exists _hints(xunits)] && "" != $_hints(xunits)} {
        set _hints(xlabel) "$_hints(xlabel) ($_hints(xunits))"
    }
    if {[info exists _hints(ylabel)] && "" != $_hints(ylabel)
        && [info exists _hints(yunits)] && "" != $_hints(yunits)} {
        set _hints(ylabel) "$_hints(ylabel) ($_hints(yunits))"
    }
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

