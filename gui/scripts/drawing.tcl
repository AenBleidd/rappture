# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: drawing - 3D drawing of data
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
    constructor {xmlobj path} {
        # defined below
    }
    destructor {
        # defined below
    }

    public method label { elem }
    public method type { elem }
    public method style { elem }
    public method shape { elem }
    public method values { elem }
    public method data { elem }
    public method hints {{keyword ""}}
    public method components { args }

    private variable _drawing
    private variable _xmlobj
    private variable _styles
    private variable _shapes
    private variable _labels
    private variable _types
    private variable _data
    private variable _hints
    private variable _units
}

# ----------------------------------------------------------------------
# Constructor
# ----------------------------------------------------------------------
itcl::body Rappture::Drawing::constructor {xmlobj path} {
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
    # determine the overall size of the device
    foreach elem [$_xmlobj children $path] {
        switch -glob -- $elem {
            # polygon is deprecated in favor of polydata
            polygon* - polydata* {
                set _data($elem) [$_xmlobj get $path.$elem.vtk]
                set _data($elem) [string trim $_data($elem)]
                set _styles($elem) [$_xmlobj get $path.$elem.about.style]
                set _labels($elem) [$_xmlobj get $path.$elem.about.label]
                set _types($elem) polydata
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
                    set contents [Rappture::PdbToVtk $pdbdata]
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
        camera  about.camera
        color   about.color
        group   about.group
        label   about.label
        type    about.type
        xdesc   xaxis.description
        ydesc   yaxis.description
        zdesc   zaxis.description
        xlabel  xaxis.label
        ylabel  yaxis.label
        zlabel  zaxis.label
        xscale  xaxis.scale
        yscale  yaxis.scale
        zscale  zaxis.scale
        xunits  xaxis.units
        yunits  yaxis.units
        zunits  zaxis.units
        xmin    xaxis.min
        xmax    xaxis.max
        ymin    yaxis.min
        ymax    yaxis.max
        zmin    zaxis.min
        zmax    zaxis.max
    } {
        set str [$_drawing get $path]
        if {"" != $str} {
            set _hints($key) $str
        }
    }
    foreach {key path} {
        toolid          tool.id
        toolname        tool.name
        toolcommand     tool.execute
        tooltitle       tool.title
        toolrevision    tool.version.application.revision
    } {
        set str [$_xmlobj get $path]
        if { "" != $str } {
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
