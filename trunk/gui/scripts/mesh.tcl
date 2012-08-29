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
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itcl

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

    private method _buildNodesElements {xmlobj path}
    private method _buildRectMesh {xmlobj path}
    private method _getVtkElement {npts}

    private variable _xmlobj ""  ;# ref to XML obj with device data
    private variable _mesh ""    ;# lib obj representing this mesh
    private variable _pts2elem   ;# maps # points => vtk element

    private variable _units "m m m" ;# system of units for x, y, z
    private variable _limits        ;# limits xmin, xmax, ymin, ymax, ...
    private variable _pdata ""      ;# name of vtkPointData object
    private variable _gdata ""      ;# name of vtkDataSet object

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
    package require vtk
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

    foreach lim {xmin xmax ymin ymax zmin zmax} {
        set _limits($lim) ""
    }

    if {[$_mesh element vtk] != ""} {
        _buildRectMesh $xmlobj $path
    } elseif {[$_mesh element node] != "" && [$_mesh element element] != ""} {
        _buildNodesElements $xmlobj $path
    } else {
        error "can't find mesh data in $path"
    }
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Mesh::destructor {} {
    # don't destroy the _xmlobj! we don't own it!
    itcl::delete object $_mesh

    catch {rename $this-points ""}
    catch {rename $this-grid ""}
    catch {rename $this-dset ""}

    foreach type [array names _pts2elem] {
        rename $_pts2elem($type) ""
    }
}

# ----------------------------------------------------------------------
# USAGE: points
#
# Returns the vtk object containing the points for this mesh.
# ----------------------------------------------------------------------
itcl::body Rappture::Mesh::points {} {
    return $_pdata
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
    return $_gdata
}

# ----------------------------------------------------------------------
# USAGE: size ?-points|-elements?
#
# Returns the number of points in this mesh.
# ----------------------------------------------------------------------
itcl::body Rappture::Mesh::size {{what -points}} {
    switch -- $what {
        -points {
            return [$_pdata GetNumberOfPoints]
        }
        -elements {
            return [$_gdata GetNumberOfCells]
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

# ----------------------------------------------------------------------
# USAGE: _buildNodesElements <xmlobj> <path>
#
# Used internally to build a mesh representation based on nodes and
# elements stored in the XML.
# ----------------------------------------------------------------------
itcl::body Rappture::Mesh::_buildNodesElements {xmlobj path} {
    # create the vtk objects containing points and connectivity
    vtkPoints $this-points
    set _pdata $this-points
    vtkUnstructuredGrid $this-grid
    set _gdata $this-grid

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
        set elem [_getVtkElement [llength $nlist]]

        set i 0
        foreach n $nlist {
            [$elem GetPointIds] SetId $i $n
            incr i
        }
        $this-grid InsertNextCell [$elem GetCellType] [$elem GetPointIds]
    }
}

# ----------------------------------------------------------------------
# USAGE: _buildRectMesh <xmlobj> <path>
#
# Used internally to build a mesh representation based on a native
# VTK file for a rectangular mesh.
# ----------------------------------------------------------------------
itcl::body Rappture::Mesh::_buildRectMesh {xmlobj path} {
    vtkRectilinearGridReader $this-gr
    $this-gr ReadFromInputStringOn
    $this-gr SetInputString [$xmlobj get $path.vtk]

    set _gdata [$this-gr GetOutput]
    set _pdata [$_gdata GetPointData]

    $_gdata Update
    foreach name {xmin xmax ymin ymax zmin zmax} val [$_gdata GetBounds] {
        set _limits($name) $val
    }
}

# ----------------------------------------------------------------------
# USAGE: _getVtkElement <npts>
#
# Used internally to find (or allocate, if necessary) a vtk element
# that can be used to build up a mesh.  The element depends on the
# number of points passed in.  4 points is a tetrahedron, 5 points
# is a pyramid, etc.
# ----------------------------------------------------------------------
itcl::body Rappture::Mesh::_getVtkElement {npts} {
    if {![info exists _pts2elem($npts)]} {
        switch -- $npts {
            3 {
                set _pts2elem($npts) $this-elem3
                vtkTriangle $_pts2elem($npts)
            }
            4 {
                set _pts2elem($npts) $this-elem4
                vtkTetra $_pts2elem($npts)
            }
            5 {
                set _pts2elem($npts) $this-elem5
                vtkPyramid $_pts2elem($npts)
            }
            6 {
                set _pts2elem($npts) $this-elem6
                vtkWedge $_pts2elem($npts)
            }
            8 {
                set _pts2elem($npts) $this-elem8
                vtkVoxel $_pts2elem($npts)
            }
        }
    }
    return $_pts2elem($npts)
}
