
# Example of using the flow object in Rappture.

package require Rappture

# Open an XML run file to write into
set driver [Rappture::library [lindex $argv 0]]

# Create 3 <mesh> and <field> objects to display the different flows.

set m1 output.mesh(jwire)
set f1 output.field(jwire)

set m2 output.mesh(half)
set f2 output.field(half)

set m3 output.mesh(sio2)
set f3 output.field(sio2)

set elements [subst {
    $f1.about.label "Jwire example"
    $f1.component.flow.label "Flow 1"
    $f1.component.flow.axis z
    $f1.component.flow.position 0%
    $f1.component.flow.volume yes
    $f1.component.flow.streams no
    $f1.component.flow.outline no
    $f1.component.flow.particles(left).label "Left particle flow"
    $f1.component.flow.particles(left).description {
	Lorem ipsum dolor sit amet, consectetur adipiscing elit. 
    }
    $f1.component.flow.particles(left).axis x 
    $f1.component.flow.particles(left).color lightgreen
    $f1.component.flow.particles(left).position 10%
    $f1.component.flow.particles(right).label "Right particle flow"
    $f1.component.flow.particles(right).description {
	Cras elit eros,
	ultricies vel, auctor nec, iaculis interdum, lectus. Vivamus lorem diam,
	posuere nec, cursus ut, tempus in, tortor. Sed est eros, faucibus eu,
	pellentesque quis, pharetra venenatis, massa.
    }
    $f1.component.flow.particles(right).axis x 
    $f1.component.flow.particles(right).color khaki
    $f1.component.flow.particles(right).position 90%
    $f1.component.style  "-color blue:red -levels 6"
    $f1.component.flow.box(one).label "Region 1"
    $f1.component.flow.box(one).color cyan
    $f1.component.flow.box(one).corner(1) "0 -100 -100" 
    $f1.component.flow.box(one).corner(2) "3000 400 400"
    $f1.component.flow.box(two).label "Region 2"
    $f1.component.flow.box(two).color violet 
    $f1.component.flow.box(two).corner(1) "1000 -150 -100" 
    $f1.component.flow.box(two).corner(2) "3000 3000 3000"
    $f1.component.flow.box(three).label "Region 3"
    $f1.component.flow.box(three).color magenta 
    $f1.component.flow.box(three).corner(1) "1000 -150 -100" 
    $f1.component.flow.box(three).corner(2) "2000 450 450"
    $f1.component.elemtype vectors
    $f1.component.elemsize 3
    $f1.component.mesh $m1
    $m1.about.label "JWire mesh"
    $m1.dim 3
    $m1.units "um"
    $m1.hide yes
    $m1.grid.xaxis.min 0
    $m1.grid.xaxis.max 6300
    $m1.grid.xaxis.numpoints 126
    $m1.grid.yaxis.min 0
    $m1.grid.yaxis.max 1500
    $m1.grid.yaxis.numpoints 30
    $m1.grid.zaxis.min 0
    $m1.grid.zaxis.max 1519.05
    $m1.grid.zaxis.numpoints 22
    $f2.about.label "Flow 2d half"
    $f2.component.flow.axis z
    $f2.component.flow.position 0%
    $f2.component.flow.volume yes
    $f2.component.flow.streams no
    $f2.component.flow.outline no
    $f2.component.flow.particles(left).axis x 
    $f2.component.flow.particles(left).color yellow
    $f2.component.flow.particles(left).position 10%
    $f2.component.flow.particles(right).axis x 
    $f2.component.flow.particles(right).color pink
    $f2.component.flow.particles(right).position 90%
    $f2.component.style  "-color rainbow -levels 6"
    $f2.camera.position {
	-qw 1 -qx 0 -qy 0 -qz 0 -xpan 0 -ypan 0 -zoom 1.0
    }
    $f2.component.elemtype vectors
    $f2.component.elemsize 3
    $f2.component.mesh $m2
    $m2.about.label "2D mesh"
    $m2.dim 2
    $m2.units "um"
    $m2.hide yes
    $m2.grid.xaxis.min -0.5
    $m2.grid.xaxis.max 152
    $m2.grid.xaxis.numpoints 305
    $m2.grid.yaxis.min -22
    $m2.grid.yaxis.max 21.6
    $m2.grid.yaxis.numpoints 109
    $f3.about.label "SiO2"
    $f3.about.view "flowvis"
    $f3.component.flow.axis z
    $f3.component.flow.position 0%
    $f3.component.flow.volume yes
    $f3.component.flow.streams no
    $f3.component.flow.outline no
    $f3.component.flow.particles(left).axis x
    $f3.component.flow.particles(left).color lightgreen
    $f3.component.flow.particles(left).position 10%
    $f3.component.flow.particles(right).axis x 
    $f3.component.flow.particles(right).color khaki
    $f3.component.flow.particles(right).position 90%
    $f3.component.style "-color blue:red -levels 6"
    $f3.component.elemtype vectors
    $f3.component.elemsize 3
    $f3.component.mesh $m3
    $m3.about.label "SiO2 Mesh"
    $m3.dim 3
    $m3.units "um"
    $m3.hide yes
    $m3.grid.xaxis.min 0
    $m3.grid.xaxis.max 29.75206608
    $m3.grid.xaxis.numpoints 121
    $m3.grid.yaxis.min -1.5
    $m3.grid.yaxis.max 2.82
    $m3.grid.yaxis.numpoints 25
    $m3.grid.zaxis.min -1
    $m3.grid.zaxis.max 4.304347828
    $m3.grid.zaxis.numpoints 23
}]

puts stdout "Setting attributes..."
flush stdout
foreach {key value} $elements {
    $driver put $key $value
}

source demo1/data-demo1.tcl
$driver put $f1.component.values $values

source demo2/data-2dflow.tcl
$driver put $f2.component.values $values

source demo3/data-values.tcl
$driver put $f3.component.values $values

# save the updated XML describing the run...
Rappture::result $driver
puts stdout "done"
flush stdout
exit 0
