
# Example of using unirect2d meshes in a field object in Rappture.

package require Rappture
package require BLT

# Read in the data since we're not simulating anything...

# Open an XML run file to write into
set driver [Rappture::library [lindex $argv 0]]

set meshtype [$driver get "input.choice(mesh).current"]
set contour [$driver get "input.boolean(contour).current"]
if { $contour  == "yes" } {
    set view contour
} else {
    set view heightmap
}

set xv [blt::vector create \#auto]
$xv seq 0 1 50

set hide no

switch -- $meshtype {
    "cloud" {
        set mesh output.mesh

        $driver put $mesh.about.label "cloud in unstructured mesh"
        $driver put $mesh.dim  2
        $driver put $mesh.units "m"
        $driver put $mesh.hide $hide

        set points {}
        foreach y [$xv range 0 end] {
            foreach x [$xv range 0 end] {
                append points "$x $y\n"
            }
        }
        $driver put $mesh.unstructured.points $points
        $driver put $mesh.unstructured.celltypes ""
    }
    "regular" {
        set mesh output.mesh

        $driver put $mesh.about.label "uniform grid mesh"
        $driver put $mesh.dim  2
        $driver put $mesh.units "m"
        $driver put $mesh.hide $hide

        $driver put $mesh.grid.xaxis.min 0.0 
        $driver put $mesh.grid.xaxis.max 1.0
        $driver put $mesh.grid.xaxis.numpoints 50
        $driver put $mesh.grid.yaxis.min 0.0 
        $driver put $mesh.grid.yaxis.max 1.0
        $driver put $mesh.grid.yaxis.numpoints 50
    }
    "irregular" {
        set mesh output.mesh

        $driver put $mesh.about.label "irregular rectilinear grid mesh"
        $driver put $mesh.dim  2
        $driver put $mesh.units "m"
        $driver put $mesh.hide $hide

        $driver put $mesh.grid.xcoords [$xv range 0 end]
        $driver put $mesh.grid.ycoords [$xv range 0 end]
    }
    "hybrid" {
        set mesh output.mesh

        $driver put $mesh.about.label "hybrid regular and irregular rectilinear grid mesh"
        $driver put $mesh.dim  2
        $driver put $mesh.units "m"
        $driver put $mesh.hide $hide

        $driver put $mesh.grid.xcoords [$xv range 0 end]
        $driver put $mesh.grid.yaxis.min 0.0 
        $driver put $mesh.grid.yaxis.max 1.0
        $driver put $mesh.grid.yaxis.numpoints 50
    }
    "structured" {
        set mesh output.mesh

        $driver put $mesh.about.label "Structured (Curvilinear) Grid"
        $driver put $mesh.dim  2
        $driver put $mesh.units "m"
        $driver put $mesh.hide $hide

        $driver put $mesh.grid.xdim 50
        $driver put $mesh.grid.ydim 50
        set points {}
        foreach y [$xv range 0 end] {
            foreach x [$xv range 0 end] {
                append points "$x $y\n"
            }
        }
        $driver put $mesh.grid.points $points
    }
    "triangular" {
        set mesh output.mesh

        $driver put $mesh.about.label "triangles in unstructured mesh"
        $driver put $mesh.dim  2
        $driver put $mesh.units "m"
        $driver put $mesh.hide $hide
        
        $driver put -type file -compress no $mesh.unstructured.points \
            points.txt
        $driver put -type file -compress no $mesh.unstructured.triangles \
            triangles.txt
    }
    "generic" {
        set mesh output.mesh

        $driver put $mesh.about.label "nodes and elements mesh"
        $driver put $mesh.dim  2
        $driver put $mesh.units "m"
        $driver put $mesh.hide $hide

        set count 0
        set f [open "points.txt" "r"]
        set points [read $f]
        close $f
        foreach { x y } $points {
            $driver put $mesh.node($count) "$x $y"
            incr count
        }
        set count 0
        set f [open "triangles.txt" "r"]
        set triangles [read $f]
        close $f
        foreach { a b c } $triangles {
            $driver put $mesh.element($count).nodes "$a $b $c"
            incr count
        }
    }
    "unstructured" {
        set mesh output.mesh

        $driver put $mesh.about.label "Unstructured Grid"
        $driver put $mesh.dim  2
        $driver put $mesh.units "m"
        $driver put $mesh.hide $hide

        $driver put -type file -compress no $mesh.unstructured.points points.txt
        set cells {}
        set f [open "triangles.txt" "r"]
        set triangles [read $f]
        close $f
        foreach { a b c } $triangles {
            append cells "$a $b $c\n"
        }
        $driver put $mesh.unstructured.cells $cells
        $driver put $mesh.unstructured.celltypes "triangle"
    }
    "cells" {
        set mesh output.mesh

        $driver put $mesh.about.label \
            "Unstructured Grid with Heterogeneous Cells"
        $driver put $mesh.dim  2
        $driver put $mesh.units "m"
        $driver put $mesh.hide $hide

        set celltypes {}
        set f [open "triangles.txt" "r"]
        set triangles [read $f]
        close $f
        foreach { a b c } $triangles {
            append cells "$a $b $c\n"
            append celltypes "triangle\n"
        }
        $driver put -type file -compress no $mesh.unstructured.points points.txt
        $driver put $mesh.unstructured.celltypes $celltypes
        $driver put $mesh.unstructured.cells $cells
    }
    "vtkmesh" {
        set mesh output.mesh

        $driver put $mesh.about.label "vtk mesh"
        $driver put $mesh.dim  2
        $driver put $mesh.units "m"
        $driver put $mesh.hide $hide
        $driver put -type file -compress no $mesh.vtk mesh.vtk
    }
    "vtkfield" {

        $driver put output.field(substrate).about.label "Substrate Surface"
        $driver put -type file -compress no \
            output.field(substrate).component.vtk file.vtk
        $driver put output.string.current ""
        Rappture::result $driver
        exit 0
    }
    default {
        error "unknown mesh type \"$meshtype\""
    }
}

$driver put output.field(substrate).about.label "Substrate Surface"
$driver put output.field(substrate).about.view $view
$driver put output.field(substrate).component.mesh $mesh
$driver put -type file -compress no output.field(substrate).component.values \
    substrate_data.txt

$driver put output.field(particle).about.label "Particle Surface"
$driver put output.field(particle).about.view $view
$driver put output.field(particle).component.mesh $mesh
$driver put -type file -compress no output.field(particle).component.values \
    particle_data.txt

$driver put output.string.about.label "Mesh XML definition"
$driver put output.string.current [$driver xml $mesh]

# save the updated XML describing the run...
Rappture::result $driver
exit 0
