
# Example of using a unirect3d mesh in a vector field
# This is inlucded for testing purposes, since unirect3d outputs are deprecated
source data-demo1.tcl

package require Rappture

set driver [Rappture::library [lindex $argv 0]]

set elements {
    output.field.about.label "Jwire example"
    output.field.component.flow.label "Flow 1"
    output.field.component.flow.axis z
    output.field.component.flow.position 0%
    output.field.component.flow.volume yes
    output.field.component.flow.streams no
    output.field.component.flow.outline no
    output.field.component.flow.particles(left).label "Left particle flow"
    output.field.component.flow.particles(left).description {
	Lorem ipsum dolor sit amet, consectetur adipiscing elit. 
    }
    output.field.component.flow.particles(left).axis x 
    output.field.component.flow.particles(left).color lightgreen
    output.field.component.flow.particles(left).position 10%
    output.field.component.flow.particles(right).label "Right particle flow"
    output.field.component.flow.particles(right).description {
	This is a description for the right particle flow...
    }
    output.field.component.flow.particles(right).axis x 
    output.field.component.flow.particles(right).color khaki
    output.field.component.flow.particles(right).position 90%
    output.field.component.style  "-color blue:red -levels 6 -opacity 1"
    output.field.component.flow.box(one).label "Region 1"
    output.field.component.flow.box(one).color cyan
    output.field.component.flow.box(one).hide yes 
    output.field.component.flow.box(one).corner(1) "0 -100 -100" 
    output.field.component.flow.box(one).corner(2) "3000 400 400"
    output.field.component.flow.box(two).label "Region 2"
    output.field.component.flow.box(two).hide yes 
    output.field.component.flow.box(two).color violet 
    output.field.component.flow.box(two).corner(1) "1000 -150 -100" 
    output.field.component.flow.box(two).corner(2) "3000 3000 3000"
    output.field.component.flow.box(three).label "Region 3"
    output.field.component.flow.box(three).hide yes 
    output.field.component.flow.box(three).color magenta 
    output.field.component.flow.box(three).corner(1) "1000 -150 -100" 
    output.field.component.flow.box(three).corner(2) "2000 450 450"
    output.field.component.mesh unirect3d
    output.field.component.extents 3
    unirect3d.xaxis.min	0
    unirect3d.xaxis.max	6300
    unirect3d.xaxis.numpoints  126
    unirect3d.yaxis.min	0
    unirect3d.yaxis.max	1500
    unirect3d.yaxis.numpoints  30
    unirect3d.zaxis.min	0
    unirect3d.zaxis.max	1519.05
    unirect3d.zaxis.numpoints  22
    unirect3d.components 3
}

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

