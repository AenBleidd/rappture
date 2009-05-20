
# Example of using unirect2d mesh in a vector field.
source data-unirect2d.tcl

package require Rappture

set driver [Rappture::library [lindex $argv 0]]

set elements {
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
    output.field.component.style  "-color rainbow -levels 6 -opacity 1"
    output.field.camera.position {
	theta 90 phi 0 psi 0 pan-x 0 pan-y 0 zoom 1.0
    }
    output.field.component.extents 3
    output.field.component.mesh unirect2d
    unirect2d.xaxis.min	-0.5
    unirect2d.xaxis.max	152
    unirect2d.xaxis.numpoints  305
    unirect2d.yaxis.min	-22
    unirect2d.yaxis.max	21.6
    unirect2d.yaxis.numpoints  109
    unirect2d.components 2
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
