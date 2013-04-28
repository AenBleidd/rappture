# -*- mode: tcl; indent-tabs-mode: nil -*- 

# ----------------------------------------------------------------------
#  COMPONENT: drawing - 2D drawing of data
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itcl
package require BLT

namespace eval Rappture { 
    # forward declaration 
}

itcl::class Rappture::Drawing {
    private variable _drawing
    private variable _xmlobj 
    private variable _actors 
    private variable _styles 
    private variable _shapes 
    private variable _labels 
    private variable _types 
    private variable _data 
    private variable _hints
    private variable _units
    private variable _limits

    constructor {xmlobj path} { 
        # defined below 
    }
    destructor { 
        # defined below 
    }
    public method limits {axis}
    public method label { elem }
    public method type { elem }
    public method style { elem }
    public method shape { elem }
    public method values { elem }
    public method data { elem }
    public method hints {{keyword ""}} 
    public method components { args } 
    private method PdbToVtk { cname contents } 
}


# ----------------------------------------------------------------------
# Constructor
# ----------------------------------------------------------------------
itcl::body Rappture::Drawing::constructor {xmlobj path} {
    package require vtk
    if {![Rappture::library isvalid $xmlobj]} {
        error "bad value \"$xmlobj\": should be Rappture::library"
    }
    set _xmlobj $xmlobj
    set _drawing [$xmlobj element -as object $path]
    set _units [$_drawing get units]

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
                set _data($elem) [$_xmlobj get $path.$elem.vtk]
		set _data($elem) [string trim $_data($elem)]
                set _styles($elem) [$_xmlobj get $path.$elem.about.style]
                set _labels($elem) [$_xmlobj get $path.$elem.about.label]
                set _types($elem) polydata
            }
            streamlines* {
                set _data($elem) [$_xmlobj get $path.$elem.vtk]
		set _data($elem) [string trim $_data($elem)]
                set _styles($elem) [$_xmlobj get $path.$elem.about.style]
                set _labels($elem) [$_xmlobj get $path.$elem.about.label]
                set _types($elem) streamlines
            }
            glyphs* {
                set _data($elem) [$_xmlobj get $path.$elem.vtk]
		set _data($elem) [string trim $_data($elem)]
                set _styles($elem) [$_xmlobj get $path.$elem.about.style]
                set _labels($elem) [$_xmlobj get $path.$elem.about.label]
                set _shapes($elem) [$_xmlobj get $path.$elem.about.shape]
                set _types($elem) glyphs
            }
            molecule* {
                set pdbdata [$_xmlobj get $path.$elem.pdb]
                if { $pdbdata != "" } {
                    set contents [PdbToVtk $elem $pdbdata]
                } else {
                    set contents [$_xmlobj get $path.$elem.vtk]
                }
		set _data($elem) [string trim $contents]
                set _styles($elem) [$_xmlobj get $path.$elem.about.style]
                set _labels($elem) [$_xmlobj get $path.$elem.about.label]
                set _types($elem) molecule
            }
        }
    }
    foreach {key path} {
        group   about.group
        label   about.label
        color   about.color
        camera  about.camera
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
        set str [$_drawing get $path]
        if {"" != $str} {
            set _hints($key) $str
        }
    }
    foreach {key} { axisorder } {
        set str [$_drawing get $key]
        if {"" != $str} {
            set _hints($key) $str
        }
    }
}

# ----------------------------------------------------------------------
# Destructor
# ----------------------------------------------------------------------
itcl::body Rappture::Drawing::destructor {} {
    # empty
}

# 
# label -- 
# 
#       Returns the label of the named drawing element.
#
itcl::body Rappture::Drawing::label { elem } {
    if { [info exists _labels($elem)] } {
        return $_labels($elem)
    } 
    return ""
}

# 
# type -- 
# 
#       Returns the type of the named drawing element.
#
itcl::body Rappture::Drawing::type { elem } {
    if { [info exists _types($elem)] } {
        return $_types($elem)
    } 
    return ""
}

# 
# style -- 
# 
#       Returns the style string of the named drawing element.
#
itcl::body Rappture::Drawing::style { elem } {
    if { [info exists _styles($elem)] } {
        return $_styles($elem)
    } 
    return ""
}

# 
# shape -- 
# 
#       Returns the shape of the glyphs in the drawing element.
#
itcl::body Rappture::Drawing::shape { elem } {
    set shape ""
    if { [info exists _shapes($elem)] } {
        return $_shapes($elem)
    } 
    switch -- $shape { 
	arrow - cone - cube - cylinder - dodecahedron - 
	icosahedron - line - octahedron - sphere - tetrahedron  {
	    return $shape
	}
	default {
	    puts stderr "unknown glyph shape \"$shape\""
	}
    }
    return ""
}

# 
# data -- 
# 
#       Returns the data of the named drawing element.
#
itcl::body Rappture::Drawing::data { elem } {
    if { [info exists _data($elem)] } {
        return $_data($elem)
    } 
    return ""
}

# ----------------------------------------------------------------------
# method values 
#       Returns a base64 encoded, gzipped Tcl list that represents the
#       Tcl command and data to recreate the uniform rectangular grid 
#       on the nanovis server.
# ----------------------------------------------------------------------
itcl::body Rappture::Drawing::values { elem } {
    if { [info exists _data($elem)] } {
        return $_data($elem)
    } 
    return ""
}

itcl::body Rappture::Drawing::components { args } {
    return [array names _data] 
}

# ----------------------------------------------------------------------
# method limits <axis>
#       Returns a list {min max} representing the limits for the 
#       specified axis.
# ----------------------------------------------------------------------
itcl::body Rappture::Drawing::limits {which} {
    set min ""
    set max ""
    foreach key [array names _data] {
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
itcl::body Rappture::Drawing::hints { {keyword ""} } {
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


itcl::body Rappture::Drawing::PdbToVtk { cname contents } {
    package require vtk

    set reader $this-datasetreader
    vtkPDBReader $reader

    # Write the contents to a file just in case it's binary.
    set tmpfile $cname[pid].pdb
    set f [open "$tmpfile" "w"]
    fconfigure $f -translation binary -encoding binary
    puts $f $contents
    close $f
    $reader SetFileName $tmpfile
    $reader Update
    file delete $tmpfile

    set output [$reader GetOutput]
    set pointData [$output GetPointData]
    set _scalars {}
    for { set i 0 } { $i < [$pointData GetNumberOfArrays] } { incr i } {
        set name [$pointData GetArrayName $i]
	lappend _scalars $name $name "???"
    }
    set tmpfile $cname[pid].vtk
    set writer $this-datasetwriter
    vtkDataSetWriter $writer
    $writer SetInputConnection [$reader GetOutputPort]
    $writer SetFileName $tmpfile
    $writer Write
    rename $reader ""
    rename $writer ""

    set f [open "$tmpfile" "r"]
    fconfigure $f -translation binary -encoding binary
    set vtkdata [read $f]
    close $f
    file delete $tmpfile
    return $vtkdata
}
