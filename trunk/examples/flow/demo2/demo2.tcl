
# Example of using unirect2d mesh with a vector field.
# This is included only for testing purposes, since unirect2d meshes are
# deprecated.
if 0 {
    # extents=2 is deprecated, since VTK needs 3D vectors, even for 2D meshes
    # Enabling this block should cause an error
    source data-unirect2d.tcl
    set extents 2
} else {
    source data-2dflow.tcl
    set extents 3
}

package require Rappture

set driver [Rappture::library [lindex $argv 0]]

set elements [subst {
    output.field.about.label "Flow 2d half"
    output.field.component.flow.axis z
    output.field.component.flow.position 0%
    output.field.component.flow.volume yes
    output.field.component.flow.streams no
    output.field.component.flow.outline no
    output.field.component.flow.particles(left).axis x 
    output.field.component.flow.particles(left).color yellow
    output.field.component.flow.particles(left).position 10%
    output.field.component.flow.particles(right).axis x 
    output.field.component.flow.particles(right).color pink
    output.field.component.flow.particles(right).position 90%
    output.field.component.flow.box(one).label "Region 1"
    output.field.component.flow.box(one).color magenta
    output.field.component.flow.box(one).corner(1) "50 -10 0"
    output.field.component.flow.box(one).corner(2) "70 20 0"
    
    output.field.component.style  "-nonuniformcolors {0.0 steelblue4 0.000001 blue 0.01 green  0.1 grey 1.0 white} -markers {1% 2% 3% 5% 8% 10% 20% 50% 80%} -opacity 1"
    output.field.camera.position {
	-qw 1 -qx 0 -qy 0 -qz 0 -xpan 0 -ypan 0 -zoom 1.0
    }
    output.field.component.extents $extents
    output.field.component.mesh unirect2d
    unirect2d.xaxis.min	-0.5
    unirect2d.xaxis.max	152
    unirect2d.xaxis.numpoints  305
    unirect2d.yaxis.min	-22
    unirect2d.yaxis.max	21.6
    unirect2d.yaxis.numpoints  109
    unirect2d.components 2
}]

puts stdout "Setting attributes for demo1"
flush stdout

foreach {key value} $elements {
    $driver put $key $value
}
$driver put output.field.component.values $values

Rappture::result $driver

puts stdout "done"
flush stdout
exit 0
