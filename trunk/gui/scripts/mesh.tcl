# ----------------------------------------------------------------------
#  COMPONENT: mesh - represents a structured mesh for a device
#
#  This object represents a mesh in an XML description of simulator
#  output.  A mesh is a structured arrangement of points, as elements
#  composed of nodes representing coordinates.  This is a little
#  different from a cloud, which is an unstructured arrangement
#  (shotgun blast) of points.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005
#  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Itcl
package require vtk

namespace eval Rappture { # forward declaration }

itcl::class Rappture::Mesh {
    constructor {xmlobj path} { # defined below }
    destructor { # defined below }

    public method points {}
    public method elements {}
    public method mesh {}
    public method size {{what -points}}
    public method dimensions {}
    public method limits {which}
    public method hints {{key ""}}

    public proc fetch {xmlobj path}
    public proc release {obj}

    private variable _xmlobj ""  ;# ref to XML obj with device data
    private variable _mesh ""    ;# lib obj representing this mesh

    private variable _units "m m m" ;# system of units for x, y, z
    private variable _limits        ;# limits xmin, xmax, ymin, ymax, ...

    private common _xp2obj       ;# used for fetch/release ref counting
    private common _obj2ref      ;# used for fetch/release ref counting
}

# ----------------------------------------------------------------------
# USAGE: Rappture::Mesh::fetch <xmlobj> <path>
#
# Clients use this instead of a constructor to fetch the Mesh for
# a particular <path> in the <xmlobj>.  When the client is done with
# the mesh, he calls "release" to decrement the reference count.
# When the mesh is no longer needed, it is cleaned up automatically.
# ----------------------------------------------------------------------
itcl::body Rappture::Mesh::fetch {xmlobj path} {
    set handle "$xmlobj|$path"
    if {[info exists _xp2obj($handle)]} {
        set obj $_xp2obj($handle)
        incr _obj2ref($obj)
        return $obj
    }

    set obj [Rappture::Mesh ::#auto $xmlobj $path]
    set _xp2obj($handle) $obj
    set _obj2ref($obj) 1
    return $obj
}

# ----------------------------------------------------------------------
# USAGE: Rappture::Mesh::release <obj>
#
# Clients call this when they're no longer using a Mesh fetched
# previously by the "fetch" proc.  This decrements the reference
# count for the mesh and destroys the object when it is no longer
# in use.
# ----------------------------------------------------------------------
itcl::body Rappture::Mesh::release {obj} {
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
itcl::body Rappture::Mesh::constructor {xmlobj path} {
    if {![Rappture::library isvalid $xmlobj]} {
        error "bad value \"$xmlobj\": should be Rappture::library"
    }
    set _xmlobj $xmlobj
    set _mesh [$xmlobj element -as object $path]

    set u [$_mesh get units]
    if {"" != $u} {
        while {[llength $u] < 3} {
            lappend u [lindex $u end]
        }
        set _units $u
    }

    # create the vtk objects containing points and connectivity
    vtkPoints $this-points
    vtkVoxel $this-vox
    vtkUnstructuredGrid $this-grid

    foreach lim {xmin xmax ymin ymax zmin zmax} {
        set _limits($lim) ""
    }

    #
    # Extract each node and add it to the points list.
    #
    foreach comp [$xmlobj children -type node $path] {
        set xyz [$xmlobj get $path.$comp]

        # make sure we have x,y,z
        while {[llength $xyz] < 3} {
            lappend xyz "0"
        }

        # extract each point and add it to the points list
        foreach {x y z} $xyz break
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
        $this-points InsertNextPoint $x $y $z
    }
    $this-grid SetPoints $this-points

    #
    # Extract each element and add it to the mesh.
    #
    foreach comp [$xmlobj children -type element $path] {
        set nlist [$_mesh get $comp.nodes]
        set i 0
        foreach n $nlist {
            [$this-vox GetPointIds] SetId $i $n
            incr i
        }
        $this-grid InsertNextCell [$this-vox GetCellType] \
            [$this-vox GetPointIds]
    }
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Mesh::destructor {} {
    # don't destroy the _xmlobj! we don't own it!
    itcl::delete object $_mesh

    rename $this-points ""
    rename $this-vox ""
    rename $this-grid ""
}

# ----------------------------------------------------------------------
# USAGE: points
#
# Returns the vtk object containing the points for this mesh.
# ----------------------------------------------------------------------
itcl::body Rappture::Mesh::points {} {
    # not implemented
    return $this-points
}

# ----------------------------------------------------------------------
# USAGE: elements
#
# Returns a list of the form {plist r plist r ...} for each element
# in this mesh.  Each plist is a list of {x y x y ...} coordinates
# for the mesh.
# ----------------------------------------------------------------------
itcl::body Rappture::Mesh::elements {} {
    # build a map for region number => region type
    foreach comp [$_mesh children -type region] {
        set id [$_mesh element -as id $comp]
        set regions($id) [$_mesh get $comp]
    }
    set regions() "unknown"

    set rlist ""
    foreach comp [$_mesh children -type element] {
        set rid [$_mesh get $comp.region]

        #
        # HACK ALERT!
        #
        # Prophet puts out nodes in a funny "Z" shaped order,
        # not in proper clockwise fashion.  Switch the last
        # two nodes for now to make them correct.
        #
        set nlist [$_mesh get $comp.nodes]
        set n2 [lindex $nlist 2]
        set n3 [lindex $nlist 3]
        set nlist [lreplace $nlist 2 3 $n3 $n2]
        lappend nlist [lindex $nlist 0]

        set plist ""
        foreach nid $nlist {
            eval lappend plist [$_mesh get node($nid)]
        }
        lappend rlist $plist $regions($rid)
    }
    return $rlist
}

# ----------------------------------------------------------------------
# USAGE: mesh
#
# Returns the vtk object representing the mesh.
# ----------------------------------------------------------------------
itcl::body Rappture::Mesh::mesh {} {
    return $this-grid
}

# ----------------------------------------------------------------------
# USAGE: size ?-points|-elements?
#
# Returns the number of points in this mesh.
# ----------------------------------------------------------------------
itcl::body Rappture::Mesh::size {{what -points}} {
    switch -- $what {
        -points {
            return [$this-points GetNumberOfPoints]
        }
        -elements {
            return [$this-points GetNumberOfCells]
        }
        default {
            error "bad option \"$what\": should be -points or -elements"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: dimensions
#
# Returns the number of dimensions for this object: 1, 2, or 3.
# ----------------------------------------------------------------------
itcl::body Rappture::Mesh::dimensions {} {
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
itcl::body Rappture::Mesh::limits {which} {
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
itcl::body Rappture::Mesh::hints {{keyword ""}} {
    foreach key {label color units} {
        set str [$_mesh get $key]
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
