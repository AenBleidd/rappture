
# Example of using unirect2d meshes in a field object in Rappture.

package require Rappture
package require BLT

# Read in the data since we're not simulating anything...
source data.tcl
source datatri.tcl

# Open an XML run file to write into
set driver [Rappture::library [lindex $argv 0]]

set meshtype [$driver get input.choice.current]

set xv [blt::vector create \#auto]
$xv seq 0 1 50

switch -- $meshtype {
    "unirect2d" {
	set mesh "output.unirect2d"

	$driver put $mesh.about.label "unirect2d mesh"
	$driver put $mesh.units "m"
	$driver put $mesh.hide "yes"

	# Create a unirect2d mesh.  The mesh will be used in the "field"
	# object below. We will specify a mesh and then provide the data
	# points to the field.

	$driver put $mesh.xaxis.label "Fermi-Dirac Factor"
	$driver put $mesh.yaxis.label "Energy"

	# The uniform grid that we are specifying is a mesh of 50 points on
	# the x-axis # and 50 points on the y-axis.  The 50 points are
	# equidistant between 0.0 and # 1.0

	# Specify the x-axis of the mesh
	$driver put $mesh.xaxis.min 0.0 
	$driver put $mesh.xaxis.max 1.0
	$driver put $mesh.xaxis.numpoints 50
	
	# Specify the y-axis of the mesh
	$driver put $mesh.yaxis.min 0.0 
	$driver put $mesh.yaxis.max 1.0
	$driver put $mesh.yaxis.numpoints 50
    }
    "oldcloud" {
	set mesh output.cloud
	$driver put $mesh.about.label "cloud (deprecated)"
	$driver put $mesh.units "m"
	$driver put $mesh.hide "yes"

	set points {}
	foreach y [$xv range 0 end] {
	    foreach x [$xv range 0 end] {
		append points "$x $y\n"
	    }
	}
    
	# Test old-style cloud element.
	$driver put $mesh.units "m"
	$driver put $mesh.points $points
    }
    "cloud" {
	set mesh output.mesh

	$driver put $mesh.about.label "cloud in mesh"
	$driver put $mesh.units "m"
	$driver put $mesh.hide "yes"

	set points {}
	foreach y [$xv range 0 end] {
	    foreach x [$xv range 0 end] {
		append points "$x $y\n"
	    }
	}
	$driver put $mesh.cloud.points $points
    }
    "regular" {
	set mesh output.mesh

	$driver put $mesh.about.label "uniform grid mesh"
	$driver put $mesh.units "m"
	$driver put $mesh.hide "yes"

	$driver put $mesh.grid.xaxis.min 0.0 
	$driver put $mesh.grid.xaxis.max 1.0
	$driver put $mesh.grid.xaxis.numpoints 50
	$driver put $mesh.grid.yaxis.min 0.0 
	$driver put $mesh.grid.yaxis.max 1.0
	$driver put $mesh.grid.yaxis.numpoints 50
    }
    "irregular" {
	set mesh output.mesh

	$driver put $mesh.about.label "irregular grid mesh"
	$driver put $mesh.units "m"
	$driver put $mesh.hide "yes"

	$driver put $mesh.grid.xcoords [$xv range 0 end]
	$driver put $mesh.grid.ycoords [$xv range 0 end]
    }
    "hybrid" {
	set mesh output.mesh

	$driver put $mesh.about.label "hybrid regular and irregular grid mesh"
	$driver put $mesh.units "m"
	$driver put $mesh.hide "yes"

	$driver put $mesh.units "m"
	$driver put $mesh.grid.xcoords [$xv range 0 end]
	$driver put $mesh.grid.yaxis.min 0.0 
	$driver put $mesh.grid.yaxis.max 1.0
	$driver put $mesh.grid.yaxis.numpoints 50
    }
    "triangular" {
	set mesh output.mesh

	$driver put $mesh.about.label "triangular mesh"
	$driver put $mesh.units "m"
	$driver put $mesh.hide "yes"

	$driver put $mesh.triangles.points $points
	$driver put $mesh.triangles.indices $triangles
    }
    "generic" {
	set mesh output.mesh

	$driver put $mesh.about.label "nodes and elements mesh"
	$driver put $mesh.units "m"
	$driver put $mesh.hide "yes"

	set count 0
	foreach { x y } $points {
	    $driver put $mesh.node($count) "$x $y"
	    incr count
	}
	set count 0
	foreach { a b c } $triangles {
	    $driver put $mesh.element($count).nodes "$a $b $c"
	    incr count
	}
    }
    "unstructured" {
	set mesh output.mesh

	$driver put $mesh.about.label "Unstructured Grid"
	$driver put $mesh.units "m"
	$driver put $mesh.hide "yes"

	$driver put $mesh.unstructured.points $points
	set celltypes {}
	set cells {}
	foreach { a b c } $triangles {
	    lappend celltypes "5"
	    append cells "3 $a $b $c\n"
	}
	$driver put $mesh.unstructured.cells $cells
	$driver put $mesh.unstructured.celltypes $celltypes
    }
    "cells" {
	set mesh output.mesh

	$driver put $mesh.about.label "homogeneous cells"
	$driver put $mesh.units "m"
	$driver put $mesh.hide "yes"

	$driver put $mesh.cells.points $points
	$driver put $mesh.cells.triangles $triangles
    }
    "vtkmesh" {
	set mesh output.mesh

	set f [open "mesh.vtk" "r"]
	set data [read $f]
	close $f

	$driver put $mesh.about.label "vtk mesh"
	$driver put $mesh.units "m"
	$driver put $mesh.hide "yes"
	$driver put $mesh.vtk $data
    }
    "vtkfield" {


	$driver put output.field(substrate).about.label "Substrate Surface"
	set f [open "file.vtk" "r"]
	set data [read $f]
	close $f
	$driver put output.field(substrate).component.vtk "$data"
	#$driver put output.field(substrate).about.view "contour"
	# save the updated XML describing the run...
	Rappture::result $driver
	exit 0
    }
    default {
	error "unknown mesh type \"$meshtype\""
    }
}

$driver put output.field(substrate).about.label "Substrate Surface"
#$driver put output.field(substrate).about.type "contour"
$driver put output.field(substrate).component.mesh $mesh
$driver put output.field(substrate).component.values $substrate_data

$driver put output.field(particle).about.label "Particle Surface"
#$driver put output.field(particle).about.type "contour"
$driver put output.field(particle).component.mesh $mesh
$driver put output.field(particle).component.values $particle_data

$driver put output.string.about.label "Mesh XML definition"
$driver put output.string.current [$driver xml $mesh]

# save the updated XML describing the run...
Rappture::result $driver
exit 0

