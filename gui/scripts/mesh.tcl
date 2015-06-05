# -*- mode: tcl; indent-tabs-mode: nil -*-
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
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itcl

namespace eval Rappture {
    # forward declaration
}

itcl::class Rappture::Mesh {
    constructor {xmlobj path} {
        # defined below
    }
    destructor {
        # defined below
    }
    public method points {}
    public method elements {}
    public method mesh {{-type "vtk"}}
    public method dimensions {}
    public method limits {which}
    public method units { axis }
    public method label { axis }
    public method hints {{key ""}}
    public method isvalid {} {
        return $_isValid
    }
    public proc fetch {xmlobj path}
    public proc release {obj}
    public method vtkdata {{what -partial}}
    public method type {} {
        return $_type
    }
    public method numpoints {} {
        return $_numPoints
    }
    public method numcells {} {
        return $_numCells
    }

    private method ReadNodesElements {path}
    private method GetDimension { path }
    private method GetDouble { path }
    private method GetInt { path }
    private method InitHints {}
    private method ReadGrid { path }
    private method ReadUnstructuredGrid { path }
    private method ReadVtk { path }
    private method WriteTriangles { path xv yv zv triangles }
    private method WriteQuads { path xv yv zv quads }
    private method WriteVertices { path xv yv zv vertices }
    private method WriteLines { path xv yv zv lines }
    private method WritePolygons { path xv yv zv polygons }
    private method WriteTriangleStrips { path xv yv zv trianglestrips }
    private method WriteTetrahedrons { path xv yv zv tetrahedrons }
    private method WriteHexahedrons { path xv yv zv hexhedrons }
    private method WriteWedges { path xv yv zv wedges }
    private method WritePyramids { path xv yv zv pyramids }
    private method WriteHybridCells { path xv yv zv cells celltypes }
    private method WritePointCloud { path xv yv zv }
    private method GetCellType { name }
    private method GetNumIndices { type }

    private variable _xmlobj "";        # Ref to XML obj with device data
    private variable _mesh "";          # Lib obj representing this mesh
    private variable _dim 0;            # Dimension of mesh (1, 2, or 3)
    private variable _type "";          # Indicates the type of mesh.
    private variable _axis2units;       # System of units for x, y, z
    private variable _axis2labels;      #
    private variable _hints
    private variable _limits;           # Array of mesh limits. Keys are
                                        # xmin, xmax, ymin, ymax, ...
    private variable _numPoints 0;      # Number of points in mesh
    private variable _numCells 0;       # Number of cells in mesh
    private variable _vtkdata "";       # Mesh in vtk file format.
    private variable _isValid 0;        # Indicates if the mesh is valid.

    private common _xp2obj;             # used for fetch/release ref counting
    private common _obj2ref;            # used for fetch/release ref counting
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

    # Initialize mesh bounds to empty
    foreach axis {x y z} {
        set _limits($axis) ""
    }
    set units [$_mesh get units]
    set first [lindex $units 0]
    foreach u $units axis { x y z } {
        if { $u != "" } {
            set _axis2units($axis) $u
        } else {
            set _axis2units($axis) $first
        }
    }
    foreach label [$_mesh get labels] axis { x y z } {
        if { $label != "" } {
            set _axis2labels($axis) $label
        } else {
            set _axis2labels($axis) [string toupper $axis]
        }
    }

    # Meshes comes in a variety of flavors
    #
    # Dimensionality is determined from the <dimension> tag.
    #
    # <vtk> described mesh
    # <element> +  <node> definitions
    # <grid>            rectangular mesh
    # <unstructured>    homogeneous cell type mesh.

    # Check that only one mesh type was defined.
    set subcount 0
    foreach cname [$_mesh children] {
        foreach type { vtk grid unstructured } {
            if { $cname == $type } {
                incr subcount
                break
            }
        }
    }
    if {[$_mesh element "node"] != "" ||
        [$_mesh element "element"] != ""} {
        incr subcount
    }

    if { $subcount == 0 } {
        puts stderr "WARNING: no mesh specified for \"$path\"."
        return
    }
    if { $subcount > 1 } {
        puts stderr "WARNING: too many mesh types specified for \"$path\"."
        return
    }
    set result 0
    if { [$_mesh element "vtk"] != ""} {
        set result [ReadVtk $path]
    } elseif {[$_mesh element "grid"] != "" } {
        set result [ReadGrid $path]
    } elseif {[$_mesh element "unstructured"] != "" } {
        set result [ReadUnstructuredGrid $path]
    } elseif {[$_mesh element "node"] != "" && [$_mesh element "element"] != ""} {
        set result [ReadNodesElements $path]
    }
    set _isValid $result
    InitHints
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Mesh::destructor {} {
    # don't destroy the _xmlobj! we don't own it!
    itcl::delete object $_mesh
}

#
# vtkdata --
#
#        This is called by the field object to generate a VTK file to send to
#        the remote render server.  Returns the vtkDataSet object containing
#        (at this point) just the mesh.  The field object doesn't know (or
#        care) what type of mesh is used.  The field object will add field
#        arrays before generating output to send to the remote render server.
#
itcl::body Rappture::Mesh::vtkdata {{what -partial}} {
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

# ----------------------------------------------------------------------
# USAGE: points
#
# Returns the vtk object containing the points for this mesh.
# ----------------------------------------------------------------------
itcl::body Rappture::Mesh::points {} {
    return ""
}

#
# units --
#
#       Returns the units of the given axis.
#
itcl::body Rappture::Mesh::units { axis } {
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
itcl::body Rappture::Mesh::label { axis } {
    if { ![info exists _axis2labels($axis)] } {
        return ""
    }
    return $_axis2labels($axis)
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
itcl::body Rappture::Mesh::mesh { {type "vtk"} } {
    switch $type {
        "vtk" {
            return ""
        }
        default {
            error "Requested mesh type \"$type\" is unknown."
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: dimensions
#
# Returns the number of dimensions for this object: 1, 2, or 3.
# ----------------------------------------------------------------------
itcl::body Rappture::Mesh::dimensions {} {
    return $_dim
}

# ----------------------------------------------------------------------
# USAGE: limits x|y|z
#
# Returns the {min max} coords for the limits of the specified axis.
# ----------------------------------------------------------------------
itcl::body Rappture::Mesh::limits {axis} {
    if {![info exists _limits($axis)]} {
        error "bad axis \"$which\": should be x, y, z"
    }
    return $_limits($axis)
}

# ----------------------------------------------------------------------
# USAGE: hints ?<keyword>?
#
# Returns a list of key/value pairs for various hints about plotting
# this field.  If a particular <keyword> is specified, then it returns
# the hint for that <keyword>, if it exists.
# ----------------------------------------------------------------------
itcl::body Rappture::Mesh::hints {{keyword ""}} {
    if {$keyword != ""} {
        if {[info exists _hints($keyword)]} {
            return $_hints($keyword)
        }
        return ""
    }
    return [array get _hints]
}

# ----------------------------------------------------------------------
# USAGE: InitHints
#
# Returns a list of key/value pairs for various hints about plotting
# this mesh.  If a particular <keyword> is specified, then it returns
# the hint for that <keyword>, if it exists.
# ----------------------------------------------------------------------
itcl::body Rappture::Mesh::InitHints {} {
    foreach {key path} {
        camera       camera.position
        color        about.color
        label        about.label
        style        about.style
        units        units
    } {
        set str [$_mesh get $path]
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

itcl::body Rappture::Mesh::GetDimension { path } {
    set string [$_xmlobj get $path.dim]
    if { $string == "" } {
        puts stderr "WARNING: no tag <dim> found in mesh \"$path\"."
        return 0
    }
    if { [scan $string "%d" _dim] == 1 } {
        if { $_dim == 1 || $_dim == 2 || $_dim == 3 } {
            return 1
        }
    }
    puts stderr "WARNING: bad <dim> tag value \"$string\": should be 1, 2 or 3."
    return 0
}

itcl::body Rappture::Mesh::GetDouble { path } {
    set string [$_xmlobj get $path]
    if { [scan $string "%g" value] != 1 } {
        puts stderr "ERROR: can't get double value \"$string\" of \"$path\""
        return 0.0
    }
    return $value
}

itcl::body Rappture::Mesh::GetInt { path } {
    set string [$_xmlobj get $path]
    if { [scan $string "%d" value] != 1 } {
        puts stderr "ERROR: can't get integer value \"$string\" of \"$path\""
        return 0.0
    }
    return $value
}

itcl::body Rappture::Mesh::ReadVtk { path } {
    set _type "vtk"

    if { ![GetDimension $path] } {
        return 0
    }
    # Create a VTK file with the mesh in it.
    set _vtkdata [$_xmlobj get $path.vtk]
    append out "# vtk DataFile Version 3.0\n"
    append out "mesh\n"
    append out "ASCII\n"
    append out "$_vtkdata\n"

     # Write the contents to a file just in case it's binary.
    set tmpfile file[pid].vtk
    set f [open "$tmpfile" "w"]
    fconfigure $f -translation binary -encoding binary
    puts $f $out
    close $f

    # Read the data back into a vtk dataset and query the bounds.
    set reader $this-datasetreader
    vtkDataSetReader $reader
    $reader SetFileName $tmpfile
    $reader Update
    set output [$reader GetOutput]
    set _numPoints [$output GetNumberOfPoints]
    set _numCells [$output GetNumberOfCells]
    foreach { xmin xmax ymin ymax zmin zmax } [$output GetBounds] break
    set _limits(x) [list $xmin $xmax]
    set _limits(y) [list $ymin $ymax]
    set _limits(z) [list $zmin $zmax]
    file delete $tmpfile
    rename $output ""
    rename $reader ""
    return 1
}

itcl::body Rappture::Mesh::ReadGrid { path } {
    set _type "grid"

    if { ![GetDimension $path] } {
        return 0
    }
    set numUniform 0
    set numRectilinear 0
    set numCurvilinear 0
    foreach axis { x y z } {
        set min    [$_xmlobj get "$path.grid.${axis}axis.min"]
        set max    [$_xmlobj get "$path.grid.${axis}axis.max"]
        set num    [$_xmlobj get "$path.grid.${axis}axis.numpoints"]
        set coords [$_xmlobj get "$path.grid.${axis}coords"]
        set dim    [$_xmlobj get "$path.grid.${axis}dim"]
        if { $min != "" && $max != "" && $num != "" && $num > 0 } {
            set ${axis}Min $min
            set ${axis}Max $max
            set ${axis}Num $num
            if {$min > $max} {
                puts stderr "ERROR: grid $axis axis minimum larger than maximum"
                return 0
            }
            incr numUniform
        } elseif { $coords != "" } {
            incr numRectilinear
            set ${axis}Coords $coords
        } elseif { $dim != "" } {
            set ${axis}Num $dim
            incr numCurvilinear
        }
    }
    set _dim [expr $numRectilinear + $numUniform + $numCurvilinear]
    if { $_dim == 0 } {
        # No data found.
        puts stderr "WARNING: bad grid \"$path\": no data found"
        return 0
    }
    if { $numCurvilinear > 0 } {
        # This is the 2D/3D curilinear case. We found <xdim>, <ydim>, or <zdim>
        if { $numRectilinear > 0 || $numUniform > 0 } {
            puts stderr "WARNING: bad grid \"$path\": can't mix curvilinear and rectilinear grids."
            return 0
        }
        set points [$_xmlobj get $path.grid.points]
        if { $points == "" } {
            puts stderr "WARNING: bad grid \"$path\": no <points> found."
            return 0
        }
        if { ![info exists xNum] } {
            puts stderr "WARNING: bad grid \"$path\": invalid dimensions for curvilinear grid: missing <xdim> from grid description."
            return 0
        }
        set all [blt::vector create \#auto]
        set xv [blt::vector create \#auto]
        set yv [blt::vector create \#auto]
        set zv [blt::vector create \#auto]
        $all set $points
        set numCoords [$all length]
        if { [info exists zNum] } {
            if { ![info exists yNum] || ![info exists xNum] } {
                puts stderr "WARNING: bad grid \"$path\": missing grid dimension"
                blt::vector destroy $all $xv $yv $zv
                return 0
            }
            set _dim 3
            set _numPoints [expr $xNum * $yNum * $zNum]
            set _numCells [expr ($xNum > 1 ? ($xNum - 1) : 1) * ($yNum > 1 ? ($yNum - 1) : 1) * ($zNum > 1 ? ($zNum - 1) : 1)]
            if { ($_numPoints*3) != $numCoords } {
                puts stderr "WARNING: bad grid \"$path\": invalid grid: \# of points does not match dimensions <xdim> * <ydim> * <zdim>"
                blt::vector destroy $all $xv $yv $zv
                return 0
            }
            if { ($numCoords % 3) != 0 } {
                puts stderr "WARNING: bad grid \"$path\": wrong \# of coordinates for 3D grid"
                blt::vector destroy $all $xv $yv $zv
                return 0
            }
            $all split $xv $yv $zv
            foreach axis {x y z} {
                set vector [set ${axis}v]
                set _limits($axis) [$vector limits]
            }
            append out "DATASET STRUCTURED_GRID\n"
            append out "DIMENSIONS $xNum $yNum $zNum\n"
            append out "POINTS $_numPoints double\n"
            foreach x [$xv range 0 end] y [$yv range 0 end] z [$zv range 0 end] {
                append out "$x $y $z\n"
            }
            set _vtkdata $out
        } elseif { [info exists yNum] } {
            if { ![info exists xNum] } {
                puts stderr "WARNING: bad grid \"$path\": missing grid dimension"
                blt::vector destroy $all $xv $yv $zv
                return 0
            }
            set _dim 2
            set _numPoints [expr $xNum * $yNum]
            set _numCells [expr ($xNum > 1 ? ($xNum - 1) : 1) * ($yNum > 1 ? ($yNum - 1) : 1)]
            if { ($_numPoints*2) != $numCoords } {
                puts stderr "WARNING: bad grid \"$path\": \# of points does not match dimensions <xdim> * <ydim>"
                blt::vector destroy $all $xv $yv $zv
                return 0
            }
            if { ($numCoords % 2) != 0 } {
                puts stderr "WARNING: bad grid \"$path\": wrong \# of coordinates for 2D grid"
                blt::vector destroy $all $xv $yv $zv
                return 0
            }
            $all split $xv $yv
            foreach axis {x y} {
                set vector [set ${axis}v]
                set _limits($axis) [$vector limits]
            }
            set _limits(z) [list 0 0]
            $zv seq 0 0 [$xv length]
            $all merge $xv $yv $zv
            append out "DATASET STRUCTURED_GRID\n"
            append out "DIMENSIONS $xNum $yNum 1\n"
            append out "POINTS $_numPoints double\n"
            foreach x [$xv range 0 end] y [$yv range 0 end] z [$zv range 0 end] {
                append out "$x $y $z\n"
            }
            set _vtkdata $out
        } else {
            set _dim 1
            set _numPoints $xNum
            set _numCells [expr $xNum - 1]
            if { $_numPoints != $numCoords } {
                puts stderr "WARNING: bad grid \"$path\": \# of points does not match <xdim>"
                blt::vector destroy $all $xv $yv $zv
                return 0
            }
            $all dup $xv
            set _limits(x) [$xv limits]
            set _limits(y) [list 0 0]
            set _limits(z) [list 0 0]
            $yv seq 0 0 [$xv length]
            $zv seq 0 0 [$xv length]
            $all merge $xv $yv $zv
            append out "DATASET STRUCTURED_GRID\n"
            append out "DIMENSIONS $xNum 1 1\n"
            append out "POINTS $_numPoints double\n"
            foreach x [$xv range 0 end] y [$yv range 0 end] z [$zv range 0 end] {
                append out "$x $y $z\n"
            }
            set _vtkdata $out
        }
        blt::vector destroy $all $xv $yv $zv
        return 1
    }
    if { $numRectilinear == 0 && $numUniform > 0} {
        # This is the special case where all axes 2D/3D are uniform.
        # This results in a STRUCTURED_POINTS
        if { $_dim == 1 } {
            if {$xNum == 1} {
                set xSpace 0
            } else {
                set xSpace [expr ($xMax - $xMin) / double($xNum - 1)]
            }
            set _numPoints $xNum
            set _numCells [expr $xNum - 1]
            append out "DATASET STRUCTURED_POINTS\n"
            append out "DIMENSIONS $xNum 1 1\n"
            append out "ORIGIN $xMin 0 0\n"
            append out "SPACING $xSpace 0 0\n"
            set _vtkdata $out
            set _limits(x) [list $xMin $xMax]
            set _limits(y) [list 0 0]
            set _limits(z) [list 0 0]
        } elseif { $_dim == 2 } {
            if {$xNum == 1} {
                set xSpace 0
            } else {
                set xSpace [expr ($xMax - $xMin) / double($xNum - 1)]
            }
            if {$yNum == 1} {
                set ySpace 0
            } else {
                set ySpace [expr ($yMax - $yMin) / double($yNum - 1)]
            }
            set _numPoints [expr $xNum * $yNum]
            set _numCells [expr ($xNum > 1 ? ($xNum - 1) : 1) * ($yNum > 1 ? ($yNum - 1) : 1)]
            append out "DATASET STRUCTURED_POINTS\n"
            append out "DIMENSIONS $xNum $yNum 1\n"
            append out "ORIGIN $xMin $yMin 0\n"
            append out "SPACING $xSpace $ySpace 0\n"
            set _vtkdata $out
            foreach axis {x y} {
                set _limits($axis) [list [set ${axis}Min] [set ${axis}Max]]
            }
            set _limits(z) [list 0 0]
        } elseif { $_dim == 3 } {
            if {$xNum == 1} {
                set xSpace 0
            } else {
                set xSpace [expr ($xMax - $xMin) / double($xNum - 1)]
            }
            if {$yNum == 1} {
                set ySpace 0
            } else {
                set ySpace [expr ($yMax - $yMin) / double($yNum - 1)]
            }
            if {$zNum == 1} {
                set zSpace 0
            } else {
                set zSpace [expr ($zMax - $zMin) / double($zNum - 1)]
            }
            set _numPoints [expr $xNum * $yNum * $zNum]
            set _numCells [expr ($xNum > 1 ? ($xNum - 1) : 1) * ($yNum > 1 ? ($yNum - 1) : 1) * ($zNum > 1 ? ($zNum - 1) : 1)]
            append out "DATASET STRUCTURED_POINTS\n"
            append out "DIMENSIONS $xNum $yNum $zNum\n"
            append out "ORIGIN $xMin $yMin $zMin\n"
            append out "SPACING $xSpace $ySpace $zSpace\n"
            set _vtkdata $out
            foreach axis {x y z} {
                set _limits($axis) [list [set ${axis}Min] [set ${axis}Max]]
            }
        } else {
            puts stderr "WARNING: bad grid \"$path\": bad dimension \"$_dim\""
            return 0
        }
        return 1
    }
    # This is the hybrid case.  Some axes are uniform, others are nonuniform.
    set xv [blt::vector create \#auto]
    if { [info exists xMin] } {
        $xv seq $xMin $xMax $xNum
    } else {
        $xv set [$_xmlobj get $path.grid.xcoords]
        set xMin [$xv min]
        set xMax [$xv max]
        set xNum [$xv length]
    }
    set yv [blt::vector create \#auto]
    if { $_dim > 1 } {
        if { [info exists yMin] } {
            $yv seq $yMin $yMax $yNum
        } else {
            $yv set [$_xmlobj get $path.grid.ycoords]
            set yMin [$yv min]
            set yMax [$yv max]
            set yNum [$yv length]
        }
    } else {
        set yNum 1
    }
    set zv [blt::vector create \#auto]
    if { $_dim == 3 } {
        if { [info exists zMin] } {
            $zv seq $zMin $zMax $zNum
        }  else {
            $zv set [$_xmlobj get $path.grid.zcoords]
            set zMin [$zv min]
            set zMax [$zv max]
            set zNum [$zv length]
        }
    } else {
        set zNum 1
    }
    if { $_dim == 3 } {
        set _numPoints [expr $xNum * $yNum * $zNum]
        set _numCells [expr ($xNum > 1 ? ($xNum - 1) : 1) * ($yNum > 1 ? ($yNum - 1) : 1) * ($zNum > 1 ? ($zNum - 1) : 1)]
        append out "DATASET RECTILINEAR_GRID\n"
        append out "DIMENSIONS $xNum $yNum $zNum\n"
        append out "X_COORDINATES $xNum double\n"
        append out [$xv range 0 end]
        append out "\n"
        append out "Y_COORDINATES $yNum double\n"
        append out [$yv range 0 end]
        append out "\n"
        append out "Z_COORDINATES $zNum double\n"
        append out [$zv range 0 end]
        append out "\n"
        set _vtkdata $out
        foreach axis {x y z} {
            if { [info exists ${axis}Min] } {
                set _limits($axis) [list [set ${axis}Min] [set ${axis}Max]]
            }
        }
    } elseif { $_dim == 2 } {
        set _numPoints [expr $xNum * $yNum]
        set _numCells [expr ($xNum > 1 ? ($xNum - 1) : 1) * ($yNum > 1 ? ($yNum - 1) : 1)]
        append out "DATASET RECTILINEAR_GRID\n"
        append out "DIMENSIONS $xNum $yNum 1\n"
        append out "X_COORDINATES $xNum double\n"
        append out [$xv range 0 end]
        append out "\n"
        append out "Y_COORDINATES $yNum double\n"
        append out [$yv range 0 end]
        append out "\n"
        append out "Z_COORDINATES 1 double\n"
        append out "0\n"
        foreach axis {x y} {
            if { [info exists ${axis}Min] } {
                set _limits($axis) [list [set ${axis}Min] [set ${axis}Max]]
            }
        }
        set _limits(z) [list 0 0]
        set _vtkdata $out
    } elseif { $_dim == 1 } {
        set _numPoints $xNum
        set _numCells [expr $xNum - 1]
        append out "DATASET RECTILINEAR_GRID\n"
        append out "DIMENSIONS $xNum 1 1\n"
        append out "X_COORDINATES $xNum double\n"
        append out [$xv range 0 end]
        append out "\n"
        append out "Y_COORDINATES 1 double\n"
        append out "0\n"
        append out "Z_COORDINATES 1 double\n"
        append out "0\n"
        if { [info exists xMin] } {
            set _limits(x) [list $xMin $xMax]
        }
        set _limits(y) [list 0 0]
        set _limits(z) [list 0 0]
        set _vtkdata $out
    } else {
        puts stderr "WARNING: bad grid \"$path\": invalid dimension \"$_dim\""
        return 0
    }
    blt::vector destroy $xv $yv $zv
    return 1
}

itcl::body Rappture::Mesh::WritePointCloud { path xv yv zv } {
    set _type "cloud"
    set _numPoints [$xv length]
    append out "DATASET POLYDATA\n"
    append out "POINTS $_numPoints double\n"
    foreach x [$xv range 0 end] y [$yv range 0 end] z [$zv range 0 end] {
        append out "$x $y $z\n"
    }
    set _vtkdata $out
    set _limits(x) [$xv limits]
    if { $_dim > 1 } {
        set _limits(y) [$yv limits]
    } else {
        set _limits(y) [list 0 0]
    }
    if { $_dim == 3 } {
        set _limits(z) [$zv limits]
    } else {
        set _limits(z) [list 0 0]
    }
    return 1
}

itcl::body Rappture::Mesh::WriteTriangles { path xv yv zv triangles } {
    set _type "triangles"
    set _numPoints [$xv length]
    set _numCells 0
    set data {}
    set celltypes {}
    foreach { a b c } $triangles {
        append data "3 $a $b $c\n"
        append celltypes "5\n"
        incr _numCells
    }
    append out "DATASET UNSTRUCTURED_GRID\n"
    append out "POINTS $_numPoints double\n"
    foreach x [$xv range 0 end] y [$yv range 0 end] z [$zv range 0 end] {
        append out "$x $y $z\n"
    }
    set count [expr $_numCells * 4]
    append out "CELLS $_numCells $count\n"
    append out $data
    append out "CELL_TYPES $_numCells\n"
    append out $celltypes
    set _limits(x) [$xv limits]
    set _limits(y) [$yv limits]
    if { $_dim == 3 } {
        set _limits(z) [$zv limits]
    } else {
        set _limits(z) [list 0 0]
    }
    set _vtkdata $out
    return 1
}

itcl::body Rappture::Mesh::WriteQuads { path xv yv zv quads } {
    set _type "quads"
    set _numPoints [$xv length]
    set _numCells 0
    set data {}
    set celltypes {}
    foreach { a b c d } $quads {
        append data "4 $a $b $c $d\n"
        append celltypes "9\n"
        incr _numCells
    }
    append out "DATASET UNSTRUCTURED_GRID\n"
    append out "POINTS $_numPoints double\n"
    foreach x [$xv range 0 end] y [$yv range 0 end] z [$zv range 0 end] {
        append out "$x $y $z\n"
    }
    set count [expr $_numCells * 5]
    append out "CELLS $_numCells $count\n"
    append out $data
    append out "CELL_TYPES $_numCells\n"
    append out $celltypes
    set _limits(x) [$xv limits]
    set _limits(y) [$yv limits]
    if { $_dim == 3 } {
        set _limits(z) [$zv limits]
    } else {
        set _limits(z) [list 0 0]
    }
    set _vtkdata $out
    return 1
}

itcl::body Rappture::Mesh::WriteVertices { path xv yv zv vertices } {
    set _type "vertices"
    set _numPoints [$xv length]
    set _numCells 0
    set data {}
    set lines [split $vertices \n]
    set count 0
    foreach { line } $lines {
        set numIndices [llength $line]
        if { $numIndices == 0 } {
            continue
        }
        append data "$numIndices $line\n"
        incr _numCells
        set count [expr $count + $numIndices + 1]
    }
    append out "DATASET POLYDATA\n"
    append out "POINTS $_numPoints double\n"
    foreach x [$xv range 0 end] y [$yv range 0 end] z [$zv range 0 end] {
        append out "$x $y $z\n"
    }
    append out "VERTICES $_numCells $count\n"
    append out $data
    set _limits(x) [$xv limits]
    set _limits(y) [$yv limits]
    if { $_dim == 3 } {
        set _limits(z) [$zv limits]
    } else {
        set _limits(z) [list 0 0]
    }
    set _vtkdata $out
    return 1
}

itcl::body Rappture::Mesh::WriteLines { path xv yv zv polylines } {
    set _type "lines"
    set _numPoints [$xv length]
    set _numCells 0
    set data {}
    set lines [split $polylines \n]
    set count 0
    foreach { line } $lines {
        set numIndices [llength $line]
        if { $numIndices == 0 } {
            continue
        }
        append data "$numIndices $line\n"
        incr _numCells
        set count [expr $count + $numIndices + 1]
    }
    append out "DATASET POLYDATA\n"
    append out "POINTS $_numPoints double\n"
    foreach x [$xv range 0 end] y [$yv range 0 end] z [$zv range 0 end] {
        append out "$x $y $z\n"
    }
    append out "LINES $_numCells $count\n"
    append out $data
    set _limits(x) [$xv limits]
    set _limits(y) [$yv limits]
    if { $_dim == 3 } {
        set _limits(z) [$zv limits]
    } else {
        set _limits(z) [list 0 0]
    }
    set _vtkdata $out
    return 1
}

itcl::body Rappture::Mesh::WritePolygons { path xv yv zv polygons } {
    set _type "polygons"
    set _numPoints [$xv length]
    set _numCells 0
    set data {}
    set lines [split $polygons \n]
    set count 0
    foreach { line } $lines {
        set numIndices [llength $line]
        if { $numIndices == 0 } {
            continue
        }
        append data "$numIndices $line\n"
        incr _numCells
        set count [expr $count + $numIndices + 1]
    }
    append out "DATASET POLYDATA\n"
    append out "POINTS $_numPoints double\n"
    foreach x [$xv range 0 end] y [$yv range 0 end] z [$zv range 0 end] {
        append out "$x $y $z\n"
    }
    append out "POLYGONS $_numCells $count\n"
    append out $data
    set _limits(x) [$xv limits]
    set _limits(y) [$yv limits]
    if { $_dim == 3 } {
        set _limits(z) [$zv limits]
    } else {
        set _limits(z) [list 0 0]
    }
    set _vtkdata $out
    return 1
}

itcl::body Rappture::Mesh::WriteTriangleStrips { path xv yv zv trianglestrips } {
    set _type "trianglestrips"
    set _numPoints [$xv length]
    set _numCells 0
    set data {}
    set lines [split $trianglestrips \n]
    set count 0
    foreach { line } $lines {
        set numIndices [llength $line]
        if { $numIndices == 0 } {
            continue
        }
        append data "$numIndices $line\n"
        incr _numCells
        set count [expr $count + $numIndices + 1]
    }
    append out "DATASET POLYDATA\n"
    append out "POINTS $_numPoints double\n"
    foreach x [$xv range 0 end] y [$yv range 0 end] z [$zv range 0 end] {
        append out "$x $y $z\n"
    }
    append out "TRIANGLE_STRIPS $_numCells $count\n"
    append out $data
    set _limits(x) [$xv limits]
    set _limits(y) [$yv limits]
    if { $_dim == 3 } {
        set _limits(z) [$zv limits]
    } else {
        set _limits(z) [list 0 0]
    }
    set _vtkdata $out
    return 1
}

itcl::body Rappture::Mesh::WriteTetrahedrons { path xv yv zv tetras } {
    set _type "tetrahedrons"
    set _numPoints [$xv length]
    set _numCells 0
    set data {}
    set celltypes {}
    foreach { a b c d } $tetras {
        append data "4 $a $b $c $d\n"
        append celltypes "10\n"
        incr _numCells
    }
    append out "DATASET UNSTRUCTURED_GRID\n"
    append out "POINTS $_numPoints double\n"
    foreach x [$xv range 0 end] y [$yv range 0 end] z [$zv range 0 end] {
        append out "$x $y $z\n"
    }
    set count [expr $_numCells * 5]
    append out "CELLS $_numCells $count\n"
    append out $data
    append out "CELL_TYPES $_numCells\n"
    append out $celltypes
    set _limits(x) [$xv limits]
    set _limits(y) [$yv limits]
    set _limits(z) [$zv limits]

    set _vtkdata $out
    return 1
}

itcl::body Rappture::Mesh::WriteHexahedrons { path xv yv zv hexas } {
    set _type "hexahedrons"
    set _numPoints [$xv length]
    set _numCells 0
    set data {}
    set celltypes {}
    foreach { a b c d e f g h } $hexas {
        append data "8 $a $b $c $d $e $f $g $h\n"
        append celltypes "12\n"
        incr _numCells
    }
    append out "DATASET UNSTRUCTURED_GRID\n"
    append out "POINTS $_numPoints double\n"
    foreach x [$xv range 0 end] y [$yv range 0 end] z [$zv range 0 end] {
        append out "$x $y $z\n"
    }
    set count [expr $_numCells * 9]
    append out "CELLS $_numCells $count\n"
    append out $data
    append out "CELL_TYPES $_numCells\n"
    append out $celltypes
    set _limits(x) [$xv limits]
    set _limits(y) [$yv limits]
    set _limits(z) [$zv limits]

    set _vtkdata $out
    return 1
}

itcl::body Rappture::Mesh::WriteWedges { path xv yv zv wedges } {
    set _type "wedges"
    set _numPoints [$xv length]
    set _numCells 0
    set data {}
    set celltypes {}
    foreach { a b c d e f } $wedges {
        append data "6 $a $b $c $d $e $f\n"
        append celltypes "13\n"
        incr _numCells
    }
    append out "DATASET UNSTRUCTURED_GRID\n"
    append out "POINTS $_numPoints double\n"
    foreach x [$xv range 0 end] y [$yv range 0 end] z [$zv range 0 end] {
        append out "$x $y $z\n"
    }
    set count [expr $_numCells * 7]
    append out "CELLS $_numCells $count\n"
    append out $data
    append out "CELL_TYPES $_numCells\n"
    append out $celltypes
    set _limits(x) [$xv limits]
    set _limits(y) [$yv limits]
    set _limits(z) [$zv limits]

    set _vtkdata $out
    return 1
}

itcl::body Rappture::Mesh::WritePyramids { path xv yv zv pyramids } {
    set _type "pyramids"
    set _numPoints [$xv length]
    set _numCells 0
    set data {}
    set celltypes {}
    foreach { a b c d e } $pyramids {
        append data "5 $a $b $c $d $e\n"
        append celltypes "14\n"
        incr _numCells
    }
    append out "DATASET UNSTRUCTURED_GRID\n"
    append out "POINTS $_numPoints double\n"
    foreach x [$xv range 0 end] y [$yv range 0 end] z [$zv range 0 end] {
        append out "$x $y $z\n"
    }
    set count [expr $_numCells * 6]
    append out "CELLS $_numCells $count\n"
    append out $data
    append out "CELL_TYPES $_numCells\n"
    append out $celltypes
    set _limits(x) [$xv limits]
    set _limits(y) [$yv limits]
    set _limits(z) [$zv limits]

    set _vtkdata $out
    return 1
}

itcl::body Rappture::Mesh::WriteHybridCells { path xv yv zv cells celltypes } {
    set _type "unstructured"
    set lines [split $cells \n]
    set numCellTypes [llength $celltypes]
    if { $numCellTypes == 1 } {
        set celltype [GetCellType $celltypes]
    }

    set _numPoints [$xv length]
    set data {}
    set count 0
    set _numCells 0
    set celltypesout {}
    foreach line $lines {
        set length [llength $line]
        if { $length == 0 } {
            continue
        }
        if { $numCellTypes > 1 } {
            set celltype [GetCellType [lindex $celltypes $_numCells]]
        }
        set numIndices [GetNumIndices $celltype]
        if { $numIndices > 0 && $numIndices != $length } {
            puts stderr "WARNING: bad unstructured grid \"$path\": wrong \# of indices specified for celltype $celltype on line \"$line\""
            return 0
        } else {
            set numIndices $length
        }
        append data "$numIndices $line\n"
        append celltypesout "$celltype\n"
        incr count $length;         # Include the indices
        incr count;                 # and the number of indices
        incr _numCells
    }
    append out "DATASET UNSTRUCTURED_GRID\n"
    append out "POINTS $_numPoints double\n"
    foreach x [$xv range 0 end] y [$yv range 0 end] z [$zv range 0 end] {
        append out "$x $y $z\n"
    }
    append out "CELLS $_numCells $count\n"
    append out $data
    append out "CELL_TYPES $_numCells\n"
    append out $celltypesout
    set _limits(x) [$xv limits]
    set _limits(y) [$yv limits]
    if { $_dim < 3 } {
        set _limits(z) [list 0 0]
    } else {
        set _limits(z) [$zv limits]
    }

    set _vtkdata $out
    return 1
}

itcl::body Rappture::Mesh::ReadUnstructuredGrid { path } {
    set _type "unstructured"

    if { ![GetDimension $path] } {
        return 0
    }
    # Step 1: Verify that there's only one cell tag of any kind.
    set numCells 0
    foreach type {
        cells
        hexahedrons
        lines
        polygons
        pyramids
        quads
        tetrahedrons
        triangles
        trianglestrips
        vertices
        wedges
    } {
        set data [$_xmlobj get $path.unstructured.$type]
        if { $data != "" } {
            incr numCells
        }
    }
    # The generic <cells> tag requires there be a <celltypes> tag too.
    set celltypes [$_xmlobj get $path.unstructured.celltypes]
    if { $numCells == 0 && $celltypes != "" } {
        puts stderr "WARNING: bad unstuctured grid \"$path\": no <cells> description found."
        return 0
    }
    if { $numCells > 1 } {
        puts stderr "WARNING: bad unstructured grid \"$path\": too many <cells>, <triangles>, <quads>... descriptions found: should be only one."
        return 0
    }
    foreach type {
        cells
        hexahedrons
        lines
        polygons
        pyramids
        quads
        tetrahedrons
        triangles
        trianglestrips
        vertices
        wedges
    } {
        set data [$_xmlobj get $path.unstructured.$type]
        if { $data != "" } {
            break
        }
    }
    # Step 2: Allow points to be specified as <points> or
    #         <xcoords>, <ycoords>, <zcoords>.  Split and convert into
    #         3 vectors, one for each coordinate.
    if { $_dim == 1 } {
        set xcoords [$_xmlobj get $path.unstructured.xcoords]
        set ycoords [$_xmlobj get $path.unstructured.ycoords]
        set zcoords [$_xmlobj get $path.unstructured.zcoords]
        set data    [$_xmlobj get $path.unstructured.points]
        if { $ycoords != "" } {
            put stderr "can't specify <ycoords> with a 1D mesh"
            return 0
        }
        if { $zcoords != "" } {
            put stderr "can't specify <zcoords> with a 1D mesh"
            return 0
        }
        if { $xcoords != "" } {
            set xv [blt::vector create \#auto]
            $xv set $xcoords
        } elseif { $data != "" } {
            Rappture::ReadPoints $data dim points
            if { $points == "" } {
                puts stderr "WARNING: bad unstructured grid \"$path\": no <points> found."
                return 0
            }
            if { $dim != 1 } {
                puts stderr "WARNING: bad unstructured grid \"$path\": \# of coordinates per point is \"$dim\": does not agree with dimension specified for mesh \"$_dim\""
                return 0
            }
            set xv [blt::vector create \#auto]
            $xv set $points
        } else {
            puts stderr "WARNING: bad unstructured grid \"$path\": no points specified."
            return 0
        }
        set yv [blt::vector create \#auto]
        set zv [blt::vector create \#auto]
        $yv seq 0 0 [$xv length];       # Make an all zeroes vector.
        $zv seq 0 0 [$xv length];       # Make an all zeroes vector.
    } elseif { $_dim == 2 } {
        set xcoords [$_xmlobj get $path.unstructured.xcoords]
        set ycoords [$_xmlobj get $path.unstructured.ycoords]
        set zcoords [$_xmlobj get $path.unstructured.zcoords]
        set data    [$_xmlobj get $path.unstructured.points]
        if { $zcoords != "" } {
            put stderr "can't specify <zcoords> with a 2D mesh"
            return 0
        }
        if { $xcoords != "" && $ycoords != "" } {
            set xv [blt::vector create \#auto]
            set yv [blt::vector create \#auto]
            $xv set $xcoords
            $yv set $ycoords
        } elseif { $data != "" } {
            Rappture::ReadPoints $data dim points
            if { $points == "" } {
                puts stderr "WARNING: bad unstructured grid \"$path\": no <points> found."
                return 0
            }
            if { $dim != 2 } {
                puts stderr "WARNING: bad unstructured grid \"$path\": \# of coordinates per point is \"$dim\": does not agree with dimension specified for mesh \"$_dim\""
                return 0
            }
            set all [blt::vector create \#auto]
            set xv [blt::vector create \#auto]
            set yv [blt::vector create \#auto]
            $all set $points
            $all split $xv $yv
            blt::vector destroy $all
        } else {
            puts stderr "WARNING: bad unstructured grid \"$path\": no points specified."
            return 0
        }
        set zv [blt::vector create \#auto]
        $zv seq 0 0 [$xv length];       # Make an all zeroes vector.
    } elseif { $_dim == 3 } {
        set xcoords [$_xmlobj get $path.unstructured.xcoords]
        set ycoords [$_xmlobj get $path.unstructured.ycoords]
        set zcoords [$_xmlobj get $path.unstructured.zcoords]
        set data    [$_xmlobj get $path.unstructured.points]
        if { $xcoords != "" && $ycoords != "" && $zcoords != "" } {
            set xv [blt::vector create \#auto]
            set yv [blt::vector create \#auto]
            set zv [blt::vector create \#auto]
            $xv set $xcoords
            $yv set $ycoords
            $zv set $zcoords
        } elseif { $data != "" }  {
            Rappture::ReadPoints $data dim points
            if { $points == "" } {
                puts stderr "WARNING: bad unstructured grid \"$path\": no <points> found."
                return 0
            }
            if { $dim != 3 } {
                puts stderr "WARNING: bad unstructured grid \"$path\": \# of coordinates per point is \"$dim\": does not agree with dimension specified for mesh \"$_dim\""
                return 0
            }
            set all [blt::vector create \#auto]
            set xv [blt::vector create \#auto]
            set yv [blt::vector create \#auto]
            set zv [blt::vector create \#auto]
            $all set $points
            $all split $xv $yv $zv
            blt::vector destroy $all
        } else {
            puts stderr "WARNING: bad unstructured grid \"$path\": no points specified."
            return 0
        }
    }
    set _numPoints [$xv length]

    # Step 3: Write the points and cells as vtk data.
    if { $numCells == 0 } {
        set result [WritePointCloud $path $xv $yv $zv]
    } elseif { $type == "cells" } {
        set cells [$_xmlobj get $path.unstructured.cells]
        if { $cells == "" } {
            puts stderr "WARNING: bad unstructured grid \"$path\": no cells found."
            return 0
        }
        set result [WriteHybridCells $path $xv $yv $zv $cells $celltypes]
    } else {
        set cmd "Write[string totitle $type]"
        set cells [$_xmlobj get $path.unstructured.$type]
        if { $cells == "" } {
            puts stderr "WARNING: bad unstructured grid \"$path\": no cells found."
            return 0
        }
        set result [$cmd $path $xv $yv $zv $cells]
    }
    # Clean up.
    blt::vector destroy $xv $yv $zv
    return $result
}

# ----------------------------------------------------------------------
# USAGE: ReadNodesElements <path>
#
# Used internally to build a mesh representation based on nodes and
# elements stored in the XML.
# ----------------------------------------------------------------------
itcl::body Rappture::Mesh::ReadNodesElements {path} {
    set _type "nodeselements"
    set count 0

    # Scan for nodes.  Each node represents a vertex.
    set data {}
    foreach cname [$_xmlobj children -type node $path] {
        append data "[$_xmlobj get $path.$cname]\n"
    }
    Rappture::ReadPoints $data _dim points
    if { $_dim == 2 } {
        set all [blt::vector create \#auto]
        set xv [blt::vector create \#auto]
        set yv [blt::vector create \#auto]
        set zv [blt::vector create \#auto]
        $all set $points
        $all split $xv $yv
        set _numPoints [$xv length]
        set _limits(x) [$xv limits]
        set _limits(y) [$yv limits]
        set _limits(z) [list 0 0]
        # 2D Dataset. All Z coordinates are 0
        $zv seq 0.0 0.0 $_numPoints
        $all merge $xv $yv $zv
        set points [$all range 0 end]
        blt::vector destroy $all $xv $yv $zv
    } elseif { $_dim == 3 } {
        set all [blt::vector create \#auto]
        set xv [blt::vector create \#auto]
        set yv [blt::vector create \#auto]
        set zv [blt::vector create \#auto]
        $all set $points
        $all split $xv $yv $zv
        set _numPoints [$xv length]
        set _limits(x) [$xv limits]
        set _limits(y) [$yv limits]
        set _limits(z) [$zv limits]
        set points [$all range 0 end]
        blt::vector destroy $all $xv $yv $zv
    } else {
        error "bad dimension \"$_dim\" for nodes mesh"
    }
    array set node2celltype {
        3 5
        4 10
        8 12
        6 13
        5 14
    }
    set count 0
    set _numCells 0
    set celltypes {}
    set data {}
    # Next scan for elements.  Each element represents a cell.
    foreach cname [$_xmlobj children -type element $path] {
        set nodeList [$_mesh get $cname.nodes]
        set numNodes [llength $nodeList]
        if { ![info exists node2celltype($numNodes)] } {
            puts stderr "WARNING: bad nodes/elements mesh \$path\": unknown number of indices \"$_numNodes\": should be 3, 4, 5, 6, or 8"
            return 0
        }
        set celltype $node2celltype($numNodes)
        append celltypes "$celltype\n"
        if { $celltype == 12 } {
            # Formerly used voxels instead of hexahedrons. We're converting
            # it here to be backward compatible and still fault-tolerant of
            # non-axis aligned cells.
            set newList {}
            foreach i { 0 1 3 2 4 5 7 6 } {
                lappend newList [lindex $nodeList $i]
            }
            set nodeList $newList
        }
        append data "$numNodes $nodeList\n"
        incr _numCells
        incr count $numNodes
        incr count;                        # One extra for the VTK celltype id.
    }

    append out "DATASET UNSTRUCTURED_GRID\n"
    append out "POINTS $_numPoints double\n"
    append out $points
    append out "\n"
    append out "CELLS $_numCells $count\n"
    append out $data
    append out "CELL_TYPES $_numCells\n"
    append out $celltypes
    set _vtkdata $out
    set _isValid 1
}

itcl::body Rappture::Mesh::GetCellType { name } {
    array set name2type {
        "vertex"          1
        "polyvertex"      2
        "line"            3
        "polyline"        4
        "triangle"        5
        "trianglestrip"   6
        "polygon"         7
        "pixel"           8
        "quad"            9
        "tetrahedron"     10
        "voxel"           11
        "hexahedron"      12
        "wedge"           13
        "pyramid"         14
        "pentagonalprism" 15
        "hexagonalprism"  16
    }
    if { [info exists name2type($name)] } {
        return $name2type($name)
    }
    if { [string is int $name] == 1 && $name >= 1 && $name <= 16 } {
        return $name
    }
    error "invalid celltype \"$name\""
}

itcl::body Rappture::Mesh::GetNumIndices { type } {
    array set type2indices {
        1       1
        2       0
        3       2
        4       0
        5       3
        6       0
        7       0
        8       4
        9       4
        10      4
        11      8
        12      8
        13      6
        14      5
        15      10
        16      12
    }
    if { [info exists type2indices($type)] } {
        return $type2indices($type)
    }
    error "invalid celltype \"$type\""
}
