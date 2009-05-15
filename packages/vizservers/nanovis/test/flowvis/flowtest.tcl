
# Example of using unirect2d meshes in a field object in Rappture.

package require Rappture

# Read in the data since we're not simulating anything...
set f [open vectorfield.tcl "r"]
set data [read $f]
close $f

# Open an XML run file to write into
set driver [Rappture::library [lindex $argv 0]]

# Create a "field" object to display the substrate surface.

set flowdemo {
    about.label "Jwire example"
    component(vf1).flow.axis z
    component(vf1).flow.position 0%
    component(vf1).flow.volume yes
    component(vf1).flow.streams no
    component(vf1).flow.outline no
    component(vf1).flow.particles(left).axis x 
    component(vf1).flow.particles(left).color lightgreen
    component(vf1).flow.particles(left).position 10%
    component(vf1).flow.particles(right).axis x 
    component(vf1).flow.particles(right).color khaki
    component(vf1).flow.particles(right).position 90%
    component(vf1).style  "-color blue:red -levels 6 -opacity 1"
    component(vf1).flow.box(one).color cyan
    component(vf1).flow.box(one).corner(1) "0 -100 -100" 
    component(vf1).flow.box(one).corner(2) "3000 400 400"
    component(vf1).flow.box(two).color yellow 
    component(vf1).flow.box(two).corner(1) "1000 -150 -100" 
    component(vf1).flow.box(two).corner(2) "3000 3000 3000"
    component(vf1).flow.box(three).color magenta 
    component(vf1).flow.box(three).corner(1) "1000 -150 -100" 
    component(vf1).flow.box(three).corner(2) "2000 450 450"
    camera.xposition {
	theta 142.84 phi 339.32 psi 0 pan-x -0.216 pan-y -0.128 zoom 0.64
    }
}

foreach {key value} $flowdemo {
    $driver put output.field.$key $value
}

$driver put output.field.component(vf1).extents 3
set data [Rappture::encoding::encode -as zb64 $data]
$driver put output.field.component(vf1).dx $data
# save the updated XML describing the run...
Rappture::result $driver
puts stdout "done"
flush stdout
exit 0

