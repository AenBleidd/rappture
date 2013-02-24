# -*- mode: tcl; indent-tabs-mode: nil -*- 
# ----------------------------------------------------------------------
#  COMPONENT: cloud - represents the mesh for a cloud of points
#
#  This object represents the mesh for a cloud of points in an XML
#  description of a device.  It simplifies the process of extracting
#  data that represent the mesh.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itcl

namespace eval Rappture { # forward declaration }

itcl::class Rappture::Cloud {
    constructor {xmlobj path} { # defined below }
    destructor { # defined below }

    public method points {}
    public method mesh {}
    public method vtkdata {}
    public method size {}
    public method dimensions {}
    public method limits {which}
    public method hints {{key ""}}
    public method numpoints {} {
	return $_numPoints
    }
    public proc fetch {xmlobj path}
    public proc release {obj}

    private variable _xmlobj ""  ;# ref to XML obj with device data
    private variable _cloud ""   ;# lib obj representing this cloud

    private variable _units "m m m" ;# system of units for x, y, z
    private variable _limits        ;# limits xmin, xmax, ymin, ymax, ...

    private common _xp2obj       ;# used for fetch/release ref counting
    private common _obj2ref      ;# used for fetch/release ref counting
    private variable _numPoints 0
    private variable _vtkdata ""
    private variable _vtkpoints ""
    private variable _points ""
    private variable _dim 0
}

# ----------------------------------------------------------------------
# USAGE: Rappture::Cloud::fetch <xmlobj> <path>
#
# Clients use this instead of a constructor to fetch the Cloud for
# a particular <path> in the <xmlobj>.  When the client is done with
# the cloud, he calls "release" to decrement the reference count.
# When the cloud is no longer needed, it is cleaned up automatically.
# ----------------------------------------------------------------------
itcl::body Rappture::Cloud::fetch {xmlobj path} {
    set handle "$xmlobj|$path"
    if {[info exists _xp2obj($handle)]} {
        set obj $_xp2obj($handle)
        incr _obj2ref($obj)
        return $obj
    }

    set obj [Rappture::Cloud ::#auto $xmlobj $path]
    set _xp2obj($handle) $obj
    set _obj2ref($obj) 1
    return $obj
}

# ----------------------------------------------------------------------
# USAGE: Rappture::Cloud::release <obj>
#
# Clients call this when they're no longer using a Cloud fetched
# previously by the "fetch" proc.  This decrements the reference
# count for the cloud and destroys the object when it is no longer
# in use.
# ----------------------------------------------------------------------
itcl::body Rappture::Cloud::release {obj} {
    if {[info exists _obj2ref($obj)]} {
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
    } else {
        error "can't find reference count for $obj"
    }
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Cloud::constructor {xmlobj path} {
    package require vtk
    if {![Rappture::library isvalid $xmlobj]} {
        error "bad value \"$xmlobj\": should be Rappture::library"
    }
    set _xmlobj $xmlobj
    set _cloud [$xmlobj element -as object $path]

    set u [$_cloud get units]
    if {"" != $u} {
        while {[llength $u] < 3} {
            lappend u [lindex $u end]
        }
        set _units $u
    }

    set data [$xmlobj get $path.points]
    Rappture::ReadPoints $data _dim values

    foreach lim {xmin xmax ymin ymax zmin zmax} {
        set _limits($lim) ""
    }
    set _numPoints 0
    set _points {}
    foreach line [split [$xmlobj get $path.points] \n] {
        if {"" == [string trim $line]} {
            continue
        }

        # make sure we have x,y,z
        while {[llength $line] < 3} {
            lappend line "0"
        }

        # extract each point and add it to the points list
        foreach {x y z} $line break
        foreach dim {x y z} units $_units {
            set v [Rappture::Units::convert [set $dim] \
                -context $units -to $units -units off]

            set $dim $v  ;# save back to real x/y/z variable

            if {"" == $_limits(${dim}min)} {
                set _limits(${dim}min) $v
                set _limits(${dim}max) $v
            } else {
                if {$v < $_limits(${dim}min)} { set _limits(${dim}min) $v }
                if {$v > $_limits(${dim}max)} { set _limits(${dim}max) $v }
            }
        }
        append _points "$x $y $z\n"
	incr _numPoints
    }
    if { $_dim == 3 } {
	set _vtkpoints [vtkPoints $this-vtkpoints]
	foreach { x y z } $_points {
	    $_vtkpoints InsertNextPoint $x $y $z
	}
    }
    append out "DATASET POLYDATA\n"
    append out "POINTS $_numPoints float\n"
    append out $_points
    set _vtkdata $out
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Cloud::destructor {} {
    # don't destroy the _xmlobj! we don't own it!
    itcl::delete object $_cloud
}

# ----------------------------------------------------------------------
# USAGE: points
#
# Returns the vtk object containing the points for this mesh.
# ----------------------------------------------------------------------
itcl::body Rappture::Cloud::points {} {
    return $_points
}

# ----------------------------------------------------------------------
# USAGE: mesh
#
# Returns the vtk object representing the mesh.
# ----------------------------------------------------------------------
itcl::body Rappture::Cloud::mesh {} {
    if { $_dim == 3 } {
	return $_vtkpoints
    }
    return $_points
}

# ----------------------------------------------------------------------
# USAGE: size
#
# Returns the number of points in this cloud.
# ----------------------------------------------------------------------
itcl::body Rappture::Cloud::size {} {
    return $_numPOints
}

# ----------------------------------------------------------------------
# USAGE: dimensions
#
# Returns the number of dimensions for this object: 1, 2, or 3.
# ----------------------------------------------------------------------
itcl::body Rappture::Cloud::dimensions {} {
    return $_dim
    # count the dimensions with real limits
    set dims 0
    foreach d {x y z} {
        if {$_limits(${d}min) != $_limits(${d}max)} {
            incr dims
        }
    }
    return $dims
}

# ----------------------------------------------------------------------
# USAGE: limits x|y|z
#
# Returns the {min max} values for the limits of the specified axis.
# ----------------------------------------------------------------------
itcl::body Rappture::Cloud::limits {which} {
    if {![info exists _limits(${which}min)]} {
        error "bad axis \"$which\": should be x, y, z"
    }
    return [list $_limits(${which}min) $_limits(${which}max)]
}

# ----------------------------------------------------------------------
# USAGE: hints ?<keyword>?
#
# Returns a list of key/value pairs for various hints about plotting
# this field.  If a particular <keyword> is specified, then it returns
# the hint for that <keyword>, if it exists.
# ----------------------------------------------------------------------
itcl::body Rappture::Cloud::hints {{keyword ""}} {
    foreach key {label color units} {
        set str [$_cloud get $key]
        if {"" != $str} {
            set hints($key) $str
        }
    }

    if {$keyword != ""} {
        if {[info exists hints($keyword)]} {
            return $hints($keyword)
        }
        return ""
    }
    return [array get hints]
}

itcl::body Rappture::Cloud::vtkdata {} {
    return $_vtkdata
}
