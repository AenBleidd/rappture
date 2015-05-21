
# Example of using dx file in a vector field.
# This is included for testing purposes, since DX file data is deprecated
source data-dx.tcl

package require Rappture

set driver [Rappture::library [lindex $argv 0]]

set elements {
    output.field.about.label "SiO2"
    output.field.component.flow.axis z
    output.field.component.flow.position 0%
    output.field.component.flow.volume yes
    output.field.component.flow.streams no
    output.field.component.flow.outline no
    output.field.component.flow.particles(left).axis x
    output.field.component.flow.particles(left).color lightgreen
    output.field.component.flow.particles(left).position 10%
    output.field.component.flow.particles(right).axis x 
    output.field.component.flow.particles(right).color khaki
    output.field.component.flow.particles(right).position 90%
    output.field.component.style  "-color blue:red -levels 6 -opacity 1"
    output.field.component.extents 3
}

puts stdout "Setting attributes for demo3"
flush stdout
foreach {key value} $elements {
    $driver put $key $value
}

set data [Rappture::encoding::encode -as zb64 "<DX>$dx"]
$driver put output.field.component.dx $data

Rappture::result $driver

puts stdout "done"
flush stdout
exit 0

