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
    private variable _xmlobj "";        # ref to XML obj with device data
    private variable _cloud "";         # lib obj representing this cloud
    private variable _units "" ;        # system of units for x, y, z
    private variable _axis2label;       #
    private variable _axis2units;       #
    private variable _limits;           # limits x, y, z
    private common _xp2obj ;            # Used for fetch/release ref counting
    private common _obj2ref ;           # Used for fetch/release ref counting
    private variable _numPoints 0
    private variable _vtkdata ""
    private variable _points ""
    private variable _dim 0
    private variable _isValid 0;        # Indicates if the data is valid.

    constructor {xmlobj path} {
        # defined below
    }
    destructor {
        # defined below
    }
    public method points {}
    public method mesh {}
    public method units { axis }
    public method label { axis }
    public method vtkdata {}
    public method size {}
    public method dimensions {}
    public method limits {which}
    public method hints {{key ""}}
    public method numpoints {} {
        return $_numPoints
    }
    public method isvalid {} {
        return $_isValid
    }
    public proc fetch {xmlobj path}
    public proc release {obj}
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
    if {![Rappture::library isvalid $xmlobj]} {
        error "bad value \"$xmlobj\": should be Rappture::library"
    }
    set _xmlobj $xmlobj
    set _cloud [$xmlobj element -as object $path]

    set _units [$_cloud get units]
    set first [lindex $_units 0]
    set list {}
    foreach u $_units axis { x y z } {
        if { $u != "" } {
            set _axis2units($axis) $u
        } else {
            set _axis2units($axis) $first
        }
        lappend list $_axis2units($axis)
    }
    set _units $list
    foreach label [$_cloud get labels] axis { x y z } {
        if { $label != "" } {
            set _axis2label($axis) $label
        } else {
            set _axis2label($axis) [string toupper $axis]
        }
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

        # Extract each point and add it to the points list
        foreach {x y z} $line break
        foreach axis {x y z} {
            # Units on point coordinates are NOT supported
            set value [set $axis]
            # Update limits
            if { ![info exists _limits($axis)] } {
                set _limits($axis) [list $value $value]
            } else {
                foreach { min max } $_limits($axis) break
                if {$value < $min} {
                    set min $value
                }
                if {$value > $max} {
                    set max $value
                }
                set _limits($axis) [list $min $max]
            }
        }
        append _points "$x $y $z\n"
        incr _numPoints
    }
    append out "DATASET POLYDATA\n"
    append out "POINTS $_numPoints double\n"
    append out $_points
    set _vtkdata $out
    if { $_numPoints == 0 } {
        return
    }
    set _dim 0
    foreach { xmin xmax } $_limits(x) break
    if { $xmax > $xmin } {
        incr _dim
    }
    foreach { ymin ymax } $_limits(y) break
    if { $ymax > $ymin } {
        incr _dim
    }
    foreach { zmin zmax } $_limits(z) break
    if { $zmax > $zmin } {
        incr _dim
    }
    set _isValid 1
    puts stderr "WARNING: The <cloud> element is deprecated.  Please use an unstructured <mesh> instead."
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
    return $_points
}

# ----------------------------------------------------------------------
# USAGE: size
#
# Returns the number of points in this cloud.
# ----------------------------------------------------------------------
itcl::body Rappture::Cloud::size {} {
    return $_numPoints
}

# ----------------------------------------------------------------------
# USAGE: dimensions
#
# Returns the number of dimensions for this object: 1, 2, or 3.
# ----------------------------------------------------------------------
itcl::body Rappture::Cloud::dimensions {} {
    return $_dim
}

# ----------------------------------------------------------------------
# USAGE: limits x|y|z
#
# Returns the {min max} values for the limits of the specified axis.
# ----------------------------------------------------------------------
itcl::body Rappture::Cloud::limits { axis } {
    if { ![info exists _limits($axis)] } {
        error "bad axis \"$axis\": should be x, y, z"
    }
    return $_limits($axis)
}

#
# units --
#
#       Returns the units of the given axis.
#
itcl::body Rappture::Cloud::units { axis } {
    if { ![info exists _axis2units($axis)] } {
        return ""
    }
    return $_axis2units($axis)
}

#
# label --
#
#       Returns the label of the given axis.
#
itcl::body Rappture::Cloud::label { axis } {
    if { ![info exists _axis2label($axis)] } {
        return ""
    }
    return $_axis2label($axis)
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
