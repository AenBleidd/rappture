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
    private variable _xmlobj ""  ;	# Ref to XML obj with device data
    private variable _mesh ""    ;	# Lib obj representing this mesh
    private variable _dim	0;	# Dimension of mesh (1, 2, or 3)
    private variable _type "";		# Indicates the type of mesh.
    private variable _units "m m m" ;	# System of units for x, y, z
    private variable _limits        ;	# Array of mesh limits. Keys are 
					# xmin, xmax, ymin, ymax, ...
    private variable _numPoints 0   ;	# # of points in mesh
    private variable _numCells 0   ;	# # of cells in mesh
    private variable _vtkdata "";	# Mesh in vtk file format.
    constructor {xmlobj path} { 
	# defined below 
    }
    destructor { 
	# defined below 
    }
    public method points {}
    public method elements {}
    public method mesh {{-type "vtk"}}
    public method size {{what -points}}
    public method dimensions {}
    public method limits {which}
    public method hints {{key ""}}

    public proc fetch {xmlobj path}
    public proc release {obj}
    public method vtkdata {}
    public method type {} {
	return $_type
    }
    public method numpoints {} {
	return $_numPoints
    }

    private method ReadNodesElements {xmlobj path}

    private variable _points ""      ;	# name of vtkPointData object

    private common _xp2obj       ;	# used for fetch/release ref counting
    private common _obj2ref      ;	# used for fetch/release ref counting
    private variable _isValid 0
    private variable _vtkoutput	""
    private variable _vtkreader	""
    private variable _vtkpoints	""
    private variable _vtkgrid	""
    private variable _xv	""
    private variable _yv	""
    private variable _zv	""
    private variable _xCoords	"";	# For the blt contour only
    private variable _yCoords	"";	# For the blt contour only
    
    private method GetDimension {} 
    private method GetDouble { path } 
    private method GetInt { path } 
    private method ReadCells { xmlobj path }
    private method ReadCloud { xmlobj path }
    private method ReadGrid { xmlobj path }
    private method ReadTriangles { xmlobj path }
    private method ReadVtk { xmlobj path }
    private method ReadUnstructuredGrid { xmlobj path }
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
    GetDimension
    set u [$_mesh get units]
    if {"" != $u} {
        while {[llength $u] < 3} {
            lappend u [lindex $u end]
        }
        set _units $u
    }

    # Meshes comes in a variety of flavors
    #
    # Dimensionality is determined from the <dimension> tag.  
    #
    # <vtk> described mesh
    # <element> +  <node> definitions
    # <cloud>		x,y coordinates or x,y,z coordinates 
    # <grid>		rectangular mesh 
    # <triangles>	triangular mesh
    # <cells>		homogeneous cell type mesh.
    # <unstructured_grid> homogeneous cell type mesh.
    #

    # Check that only one mesh type was defined.
    set subcount 0
    foreach cname [$_mesh children] {
	foreach type { vtk cloud grid triangles cells unstructured_grid} {
	    if { $cname == $type } {
		incr subcount
		break
	    } 
	}
    }
    set elemcount 0
    foreach cname [$_mesh children] {
	foreach type { node element } {
	    if { $cname == $type } {
		incr elemcount
		break
	    } 
	}
    }
    if { $elemcount > 0 } {
	incr $subcount
    }
    if { $subcount > 1 } {
	puts stderr "too many mesh types specified: picking first found."
    }
    if { [$_mesh element "vtk"] != ""} {
	ReadVtk $xmlobj $path
    } elseif { [$_mesh element "cloud"] != "" } {
	ReadCloud $xmlobj $path
    } elseif {[$_mesh element "grid"] != "" } {
	ReadGrid $xmlobj $path 
    } elseif {[$_mesh element "triangles"] != "" } {
	ReadTriangles $xmlobj $path
    } elseif {[$_mesh element "cells"] != "" } {
	ReadCells $xmlobj $path
    } elseif {[$_mesh element "unstructured_grid"] != "" } {
	ReadUnstructuredGrid $xmlobj $path
    } elseif {[$_mesh element "node"] != "" && [$_mesh element "element"]!=""} {
        ReadNodesElements $xmlobj $path
    }
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Mesh::destructor {} {
    # don't destroy the _xmlobj! we don't own it!
    itcl::delete object $_mesh

    if { $_xCoords != "" } {
	blt::vector destroy $_xCoords
    }
    if { $_yCoords != "" } {
	blt::vector destroy $_yCoords
    }
    if { $_vtkoutput != "" } {
	rename $_vtkoutput ""
    }
    if { $_vtkreader != "" } {
	rename $_vtkreader ""
    }
    if { $_points != "" } {
	rename $_points ""
    }
}

#
# vtkdata -- 
#
#	This is called by the field object to generate a VTK file to send to
#	the remote render server.  Returns the vtkDataSet object containing
#	(at this point) just the mesh.  The field object doesn't know (or
#	care) what type of mesh is used.  The field object will add field
#	arrays before generating output to send to the remote render server.
#
itcl::body Rappture::Mesh::vtkdata {} {
    return $_vtkdata
}

# ----------------------------------------------------------------------
# USAGE: points
#
# Returns the vtk object containing the points for this mesh.
# ----------------------------------------------------------------------
itcl::body Rappture::Mesh::points {} {
    return $_points
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
	    return $_vtkgrid 
	}
	default { 
	    error "Requested mesh type \"$type\" is unknown."
	}
    }
}

# ----------------------------------------------------------------------
# USAGE: size ?-points|-elements?
#
# Returns the number of points in this mesh.
# ----------------------------------------------------------------------
itcl::body Rappture::Mesh::size {{what -points}} {
    switch -- $what {
        -points {
            return $_numPoints
        }
        -elements {
            return $_numCells
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

itcl::body Rappture::Mesh::GetDimension {} {
    set string [$_xmlobj get dimension]
    if { [scan $string "%d" _dim] != 1 } {
	set _dim -1
	return
    }
    if { $_dim < 1 || $_dim > 4 } {
	error "bad dimension in mesh"
    }  
}

itcl::body Rappture::Mesh::GetDouble { path } {
    set string [$_xmlobj get $path]
    if { [scan $string "%g" value] != 1 } {
        puts stderr "can't get double value \"$string\" of \"$path\""
        return 0.0
    }
    return $value
}

itcl::body Rappture::Mesh::GetInt { path } {
    set string [$_xmlobj get $path]
    if { [scan $string "%d" value] != 1 } {
        puts stderr "can't get integer value \"$string\" of \"$path\""
        return 0.0
    }
    return $value
}

itcl::body Rappture::Mesh::ReadCloud { xmlobj path } {
    set _type "cloud"
    set data [$xmlobj get $path.cloud.points] 
    Rappture::ReadPoints $data _dim coords
	
    set all [blt::vector create \#auto]
    $all set $coords 
    set xv [blt::vector create \#auto]
    set yv [blt::vector create \#auto]
    set zv [blt::vector create \#auto]
    if { $_dim == 2 } {
	$all split $xv $yv
	$zv seq 0 0 [$xv length]
    } elseif { $_dim == 3 } {
	$all split $xv $yv $zv
    } else {
	error "invalid dimension \"$_dim\" of cloud mesh"
    }
    set _numPoints [$xv length]
    if { $_dim == 2 } {
	append out "DATASET POLYDATA\n"
	append out "POINTS $_numPoints float\n"
	foreach x [$xv range 0 end] y [$yv range 0 end]  {
	    append out "$x $y 0\n"
	}
	set _vtkdata $out
	foreach axis {x y} {
	    set vector [set ${axis}v]
	    set _limits($axis) [$vector limits]
	}
    } elseif { $_dim == 3 } {
	append out "DATASET POLYDATA\n"
	append out "POINTS $_numPoints float\n"
	foreach x [$xv range 0 end] y [$yv range 0 end] z [$zv range 0 end] {
	    append out "$x $y $z\n"
	}
	set _vtkdata $out
	foreach axis {x y z} {
	    set vector [set ${axis}v]
	    set _limits($axis) [$vector limits]
	}
    }
    blt::vector destroy $xv $yv $zv $all
}

itcl::body Rappture::Mesh::ReadVtk { xmlobj path } {
    set _type "vtk"

    # Create a VTK file with the mesh in it.  
    set _vtkdata [$xmlobj get $path.vtk]
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
    foreach { xmin xmax ymin ymax zmin zmax } [$output GetBounds] break
    set _dim 3;				# Hard coding all vtk meshes to 3D?
    set _limits(x) [list $xmin $xmax]
    set _limits(y) [list $ymin $ymax]
    set _limits(z) [list $zmin $zmax]
    if { $zmin >= $zmax } {
	set _dim 2
    } 
    file delete $tmpfile
    rename $output ""
    rename $reader ""
    set _isValid 1 
}

itcl::body Rappture::Mesh::ReadGrid { xmlobj path } {
    set _type "grid"
    set numUniform 0
    set numRectilinear 0
    set numCurvilinear 0
    foreach axis { x y z } {
	set min    [$xmlobj get "$path.grid.${axis}axis.min"]
	set max    [$xmlobj get "$path.grid.${axis}axis.max"]
	set num    [$xmlobj get "$path.grid.${axis}axis.numpoints"]
	set coords [$xmlobj get "$path.grid.${axis}coords"]
	set dim    [$xmlobj get "$path.grid.${axis}dim"]
	if { $min != "" && $max != "" && $num != "" && $num > 0 } {
	    set ${axis}Min $min
	    set ${axis}Max $max
	    set ${axis}Num $num
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
	return 0
    }
    if { $numCurvilinear > 0 } {
        # This is the 2D/3D curilinear case. We found <xdim>, <ydim>, or <zdim>
        if { $numRectilinear > 0 || $numUniform > 0 } {
            error "can't mix curvilinear and rectilinear grids."
        }
        if { $numCurvilinear < 2 } {
            error "curvilinear grid must be 2D or 3D."
        }
        set points [$xmlobj get $path.grid.points]
        if { $points == "" } {
            error "missing <points> from grid description."
        }
	if { ![info exists xNum] || ![info exists yNum] } {
            error "invalid dimensions for curvilinear grid: missing <xdim> or <ydim> from grid description."
        }
        set all [blt::vector create \#auto]
        set xv [blt::vector create \#auto]
        set yv [blt::vector create \#auto]
        set zv [blt::vector create \#auto]
        $all set $points
        set numCoords [$all length]
        if { [info exists zNum] } {
            set _dim 3
	    set _numPoints [expr $xNum * $yNum * $zNum]
            $all set $points
            if { ($_numPoints*3) != $numCoords } {
                error "invalid grid: \# of points does not match dimensions <xdim> * <ydim>"
            }
            if { ($numCoords % 3) != 0 } {
                error "wrong \# of coordinates for 3D grid"
            }
            $all split $xv $yv $zv
	    foreach axis {x y z} {
                set vector [set ${axis}v]
                set _limits($axis) [$vector limits]
	    }
	    append out "DATASET STRUCTURED_GRID\n"
	    append out "DIMENSIONS $xNum $yNum $zNum\n"
	    append out "POINTS $_numPoints double\n"
            append out [$all range 0 end]
            append out "\n"
	    set _vtkdata $out
        } else {
            set _dim 2
	    set _numPoints [expr $xNum * $yNum]
            if { ($_numPoints*2) != $numCoords } {
                error "invalid grid: \# of points does not match dimensions <xdim> * <ydim> * <zdim>"
            }
            if { ($numCoords % 2) != 0 } {
                error "wrong \# of coordinates for 2D grid"
            }
	    foreach axis {x y} {
                set vector [set ${axis}v]
                set _limits($axis) [$vector limits]
	    }
            $zv seq 0 0 [$xv length]
            $all merge $xv $yv $zv
	    append out "DATASET STRUCTURED_GRID\n"
	    append out "DIMENSIONS $xNum $yNum 1\n"
	    append out "POINTS $_numPoints double\n"
            append out [$all range 0 end]
            append out "\n"
	    set _vtkdata $out
	}
        blt::vector destroy $all $xv $yv $zv
	set _isValid 1 
	return 1
    }
    if { $numRectilinear == 0 && $numUniform > 0} {
	# This is the special case where all axes 2D/3D are uniform.  
        # This results in a STRUCTURE_POINTS
	if { $_dim == 2 } {
	    set xSpace [expr ($xMax - $xMin) / double($xNum - 1)]
	    set ySpace [expr ($yMax - $yMin) / double($yNum - 1)]
	    set _numPoints [expr $xNum * $yNum]
	    append out "DATASET STRUCTURED_POINTS\n"
	    append out "DIMENSIONS $xNum $yNum 1\n"
	    append out "SPACING $xSpace $ySpace 0\n"
	    append out "ORIGIN 0 0 0\n"
	    set _vtkdata $out
	    foreach axis {x y} {
		set _limits($axis) [list [set ${axis}Min] [set ${axis}Max]]
	    }
	} elseif { $_dim == 3 } {
	    set xSpace [expr ($xMax - $xMin) / double($xNum - 1)]
	    set ySpace [expr ($yMax - $yMin) / double($yNum - 1)]
	    set zSpace [expr ($zMax - $zMin) / double($zNum - 1)]
	    set _numPoints [expr $xNum * $yNum * zNum]
	    append out "DATASET STRUCTURED_POINTS\n"
	    append out "DIMENSIONS $xNum $yNum $zNum\n"
	    append out "SPACING $xSpace $ySpace $zSpace\n"
	    append out "ORIGIN 0 0 0\n"
	    set _vtkdata $out
	    foreach axis {x y z} {
		set _limits($axis) [list [set ${axis}Min] [set ${axis}Max]]
	    }
	} else { 
	    error "bad dimension of mesh \"$_dim\""
	}
	set _isValid 1 
	return 1
    }
    # This is the hybrid case.  Some axes are uniform, others are nonuniform.
    set xv [blt::vector create \#auto]
    if { [info exists xMin] } {
	$xv seq $xMin $xMax $xNum
    } else {
	$xv set [$xmlobj get $path.grid.xcoords]
	set xMin [$xv min]
	set xMax [$xv max]
	set xNum [$xv length]
    }
    set yv [blt::vector create \#auto]
    if { [info exists yMin] } {
	$yv seq $yMin $yMax $yNum
    } else {
	$yv set [$xmlobj get $path.grid.ycoords]
	set yMin [$yv min]
	set yMax [$yv max]
	set yNum [$yv length]
    }
    set zv [blt::vector create \#auto]
    if { $_dim == 3 } {
	if { [info exists zMin] } {
	    $zv seq $zMin $zMax $zNum
	}  else {
	    $zv set [$xmlobj get $path.grid.zcoords]
	    set zMin [$zv min]
	    set zMax [$zv max]
	    set zNum [$zv length]
	}
    } else {
	set zNum 1
    }
    if { $_dim == 3 } {
	set _numPoints [expr $xNum * $yNum * $zNum]
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
	set _vtkdata $out
    } else {
	error "invalid dimension of grid \"$_dim\""
    }
    blt::vector destroy $xv $yv $zv 
    set _isValid 1 
    return 1
}

itcl::body Rappture::Mesh::ReadCells { xmlobj path } {
    set _type "cells"

    set data [$xmlobj get $path.cells.points]
    Rappture::ReadPoints $data _dim points
    if { $points == "" } {
	puts stderr "no <points> found for <cells> mesh"
	return 0
    }
    array set celltype2vertices {
	triangles 3
	quads	 4
	tetraheadrons 4
	voxels 8
	hexaheadrons 8
	wedges 6
	pyramids 5
    }
    array set celltype2vtkid {
	triangles 5
	quads	 9
	tetraheadrons 10
	voxels 11
	hexaheadrons 12
	wedges 13
	pyramids 14
    }
    set count 0

    # Try to detect the celltype used.  There should be a single XML tag 
    # containing all the cells defined.  It's an error if there's more than
    # one of these, or if there isn't one.  
    foreach type { 
	triangles quads tetraheadrons voxels hexaheadrons wedges pyramids }  {
	if { [$xmlobj get $path.cells.$type] != "" } {
	    set celltype $type
	    incr count
	}
    }
    if { $count == 0 } {
	puts stderr "no cell types found"
	return 0
    }
    if { $count > 1 } {
	puts stderr "too many cell types tags found"
	return 0
    }
    set cells [$xmlobj get $path.cells.$celltype]
    if { $cells == "" } {
	puts stderr "no data for celltype \"$celltype\""
	return 0
    }
    # Get the number of vertices for each cell and the corresponding VTK id.
    set numVertices $celltype2vertices($celltype)
    set vtkid       $celltype2vtkid($celltype)
    
    if { $_dim == 2 } {
	# Load the points into one big vector, then split out the x and y
	# coordinates.
	set all [blt::vector create \#auto]
	$all set $points
	set xv [blt::vector create \#auto]
	set yv [blt::vector create \#auto]
	$all split $xv $yv 
	set _numPoints [$xv length]

	set data {}
	set _numCells 0
	set celltypes {}
	# Build cell information.  Look line by line.  Most cell types
	# have a specific number of vertices, but there are a few that
	# have a variable number of vertices.
	foreach line [split $cells \n] {
	    set numVerticies [llength $line]
	    if { $numVerticies == 0 } {
		continue;		# Skip blank lines
	    }
	    append data "    $numVertices $line\n"
	    append celltypes "$vtkid\n"
	    incr _numCells
	}
	append out "DATASET UNSTRUCTURED_GRID\n"
	append out "POINTS $_numPoints float\n"
	foreach x [$xv range 0 end] y [$yv range 0 end] {
	    append out "    $x $y 0\n"
	}
	append out "CELLS $_numCells [expr $_numCells * ($numVertices + 1)]\n"
	append out $data
	append out "CELL_TYPES $_numCells\n"
	append out $celltypes
	set _limits(x) [$xv limits]
	set _limits(y) [$yv limits]
	blt::vector destroy $xv $yv $all
    } else {
	# Load the points into one big vector, then split out the x, y, and z
	# coordinates.
	set all [blt::vector create \#auto]
	$all set $points
	set xv [blt::vector create \#auto]
	set yv [blt::vector create \#auto]
	set zv [blt::vector create \#auto]
	$all split $xv $yv $zv
	set _numPoints [$xv length]

	set data {}
	set _numCells 0
	set celltypes {}
	# Build cell information.  Look line by line.  Most cell types
	# have a specific number of vertices, but there are a few that
	# have a variable number of vertices.
	foreach line [split $cells \n] {
	    set numVerticies [llength $line]
	    if { $numVerticies == 0 } {
		continue;		# Skip blank lines
	    }
	    append data "    $numVertices $line\n"
	    append celltypes "$vtkid\n"
	    incr _numCells
	}
	append out "DATASET UNSTRUCTURED_GRID\n"
	append out "POINTS $_numPoints float\n"
	foreach x [$xv range 0 end] y [$yv range 0 end] z [$zv range 0 end] {
	    append out "    $x $y $z\n"
	}
	append out "CELLS $_numCells [expr $_numCells * ($numVerticies + 1)]\n"
	append out $data
	append out "CELL_TYPES $_numCells\n"
	append out $celltypes
	set _limits(x) [$xv limits]
	set _limits(y) [$yv limits]
	set _limits(z) [$zv limits]
	blt::vector destroy $xv $yv $zv $all
    }
    set _vtkdata $out
    set _isValid 1 
}


itcl::body Rappture::Mesh::ReadTriangles { xmlobj path } {
    set _type "triangles"
    set _dim 2
    set triangles [$xmlobj get $path.triangles.indices]
    if { $triangles == "" } { 
	puts stderr "no triangle indices specified in mesh"
	return 0
    }
    set xcoords [$xmlobj get $path.triangles.xcoords]
    set ycoords [$xmlobj get $path.triangles.ycoords]
    set points [$xmlobj get $path.triangles.points]
    set xv [blt::vector create \#auto]
    set yv [blt::vector create \#auto]
    if { $xcoords != "" && $ycoords != "" } {
	$xv set $xcoords
	$yx set $ycoords
    } elseif { $points != "" } { 
	set all [blt::vector create \#auto]
	$all set $points
	$all split $xv $yv
    } else {
	puts stderr "missing either <xcoords>, <ycoords>, or <points> for triangular mesh"
	return 0
    }
    set _numPoints [$xv length]
    set count 0
    set data {}
    foreach { a b c } $triangles {
	append data "    3 $a $b $c\n"
	incr count
    }
    set celltypes {}
    for { set i 0 } { $i < $count } { incr i } {
	append celltypes "5\n"
    }
    append out "DATASET UNSTRUCTURED_GRID\n"
    append out "POINTS $_numPoints float\n"
    foreach x [$xv range 0 end] y [$yv range 0 end] {
	append out "    $x $y 0\n"
    }
    append out "CELLS $count [expr $count * 4]\n"
    append out $data
    append out "CELL_TYPES $count\n"
    append out $celltypes
    set _limits(x) [$xv limits]
    set _limits(y) [$yv limits]
    set _vtkdata $out
    set _isValid 1 
}

itcl::body Rappture::Mesh::ReadUnstructuredGrid { xmlobj path } {
    set _type "unstructured_grid"

    set data [$xmlobj get $path.unstructured_grid.points]
    Rappture::ReadPoints $data _dim points
    if { $points == "" } {
	puts stderr "no <points> found for <cells> mesh"
	return 0
    }
    set cells [$xmlobj get $path.unstructured_grid.cells]
    if { $cells == "" } {
	puts stderr "no <cells> description found unstructured_grid"
	return 0
    }
    set celltypes [$xmlobj get $path.unstructured_grid.celltypes]
    if { $cells == "" } {
	puts stderr "no <cells> description found unstructured_grid."
	return 0
    }
    set lines [split $cells \n]
    
    set all [blt::vector create \#auto]
    set xv [blt::vector create \#auto]
    set yv [blt::vector create \#auto]
    set zv [blt::vector create \#auto]
    $all set $points
    if { $_dim == 2 } {
	# Load the points into one big vector, then split out the x and y
	# coordinates.
	$all split $xv $yv 
	set _numPoints [$xv length]

	set data {}
        set count 0
        set _numCells 0
	foreach line $lines {
            set length [llength $line]
	    if { $length == 0 } {
		continue
	    }
            set celltype [lindex $celltypes $_numCells]
            if { $celltype == "" } {
                puts stderr "can't find celltype for line \"$line\""
                return 0
            }
	    append data " $line\n"
            incr count $length
            incr _numCells
	}
	append out "DATASET UNSTRUCTURED_GRID\n"
	append out "POINTS $_numPoints float\n"
        $zv seq 0 0 [$xv length];       # Make an all zeroes vector.
	$all merge $xv $yv $zv
        append out [$all range 0 end]
	append out "CELLS $_numCells $count\n"
	append out $data
	append out "CELL_TYPES $_numCells\n"
	append out $celltypes
	set _limits(x) [$xv limits]
	set _limits(y) [$yv limits]
    } else {
	# Load the points into one big vector, then split out the x, y, and z
	# coordinates.
	$all split $xv $yv $zv
	set _numPoints [$xv length]

	set data {}
        set count 0
        set _numCells 0
	foreach line $lines {
            set length [llength $line]
	    if { $length == 0 } {
		continue
	    }
            set celltype [lindex $celltypes $_numCells]
            if { $celltype == "" } {
                puts stderr "can't find celltype for line \"$line\""
                return 0
            }
	    append data " $line\n"
            incr count $length
            incr _numCells
	}
	append out "DATASET UNSTRUCTURED_GRID\n"
	append out "POINTS $_numPoints float\n"
        $all merge $xv $yv $zv
        append out [$all range 0 end]
        append out "\n"
	append out "CELLS $_numCells $count\n"
	append out $data
	append out "CELL_TYPES $_numCells\n"
	append out $celltypes
	set _limits(x) [$xv limits]
	set _limits(y) [$yv limits]
	set _limits(z) [$zv limits]
    }
    blt::vector destroy $xv $yv $zv $all
    set _vtkdata $out
    set _isValid 1 
}

# ----------------------------------------------------------------------
# USAGE: ReadNodesElements <xmlobj> <path>
#
# Used internally to build a mesh representation based on nodes and
# elements stored in the XML.
# ----------------------------------------------------------------------
itcl::body Rappture::Mesh::ReadNodesElements {xmlobj path} {
    set type "nodeselements"
    set count 0

    # Scan for nodes.  Each node represents a vertex.
    set data {}
    foreach cname [$xmlobj children -type node $path] {
	append data "[$xmlobj get $path.$cname]\n"
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
	foreach axis {x y} {
	    set vector [set ${axis}v]
	    set _limits($axis) [${vector} limits]
	}
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
	foreach axis {x y z} {
	    set vector [set ${axis}v]
	    set _limits($axis) [${vector} limits]
	}
	set points [$all range 0 end]
	blt::vector destroy $all $xv $yv $zv
    } else {
	error "bad dimension \"$_dim\" for nodes mesh"
    }
    array set node2celltype {
	3 5
	4 10
	8 11
	6 13
	5 14
    }
    set count 0
    set _numCells 0
    set celltypes {}
    set data {}
    # Next scan for elements.  Each element represents a cell.
    foreach cname [$xmlobj children -type element $path] {
        set nodeList [$_mesh get $cname.nodes]
	set numNodes [llength $nodeList]
	if { ![info exists node2celltype($numNodes)] } {
	    puts stderr "unknown number of indices \"$_numNodes\": should be 3, 4, 5, 6, or 8"
	    return 0
	}
	set celltype $node2celltype($numNodes)
	append celltypes "  $celltype\n"
	append data "  $numNodes $nodeList\n"
	incr _numCells
	incr count $numNodes 
 	incr count;			# One extra for the VTK celltype id.
    }

    append out "DATASET UNSTRUCTURED_GRID\n"
    append out "POINTS $_numPoints float\n"
    append out $points
    append out "CELLS $_numCells $count\n"
    append out $data
    append out "CELL_TYPES $_numCells\n"
    append out $celltypes
    append out "\n"
    set _vtkdata $out
    set _isValid 1 
}
