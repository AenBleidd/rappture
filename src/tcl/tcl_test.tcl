package require Rappture

Rappture::library "rplib_test.xml"

puts [library0 get input.(max).current]
puts [library0 get input.(max)]

library0 put -append yes -id 1 input.(max).current "9"

puts [library0 xml]

set outp "\nTESTING ELEMENT"
append outp "\nelement -as path input.(max): "
append outp [library0 element -as path input.(max)]
append outp "\nelement -as component input.(max): " 
append outp [library0 element -as component input.(max)]
append outp "\nelement -as type input.(max): "
append outp [library0 element -as type input.(max)]
append outp "\nelement -as id input.(max): "
append outp [library0 element -as id input.(max)]
append outp "\nelement -as object input.(max): "
append outp [library0 element -as object input.(max)]
puts $outp

set outp "\nTESTING PARENT"
append outp "\nparent  -as path input.(max).current: "
append outp [library0 parent  -as path input.(max).current]
append outp "\nparent  -as component input.(max).current: "
append outp [library0 parent  -as component input.(max).current]
append outp "\nparent  -as type input.(max).current: "
append outp [library0 parent  -as type input.(max).current]
append outp "\nparent  -as id input.(max).current: "
append outp [library0 parent  -as id input.(max).current]
append outp "\nparent  -as object input.(max).current: "
append outp [library0 parent  -as object input.(max).current]

append outp "\nparent  input.(max): "
append outp [library0 parent  input.(max)]
append outp "\nparent  input: "
append outp [library0 parent  input]
append outp "\nparent  current: "
append outp [library0 parent  current]
puts $outp

set outp "\nTESTING CHILDREN"
append outp "\nchildren input: "
append outp [library0 children input]
append outp "\nchildren -as path input: "
append outp [library0 children -as path input]
append outp "\nchildren -as id -type number input: "
append outp [library0 children -as id -type number input]
append outp "\nchildren -as object input: "
append outp [library0 children -as object input]
puts $outp

puts "Copying min and max"
library0 copy input.test from input.(min)
library0 copy input.test from input.(max)
puts [library0 xml]

puts "Removing min and max"
library0 remove input.test.(min)
library0 remove input.test.(max)
puts [library0 xml]

puts "Removing library0"
library0 remove

puts "Printing XML"
puts [library0 xml]

puts "opening new library"
set libObj1 [Rappture::library "rplib_test.xml"]
set libObj2 [Rappture::library "rplib_test.xml"]
puts "libObj1 = "
puts $libObj1

puts "COPYING BETWEEN LIBS"
$libObj1 copy "input.test" from libObj2 "input.(max)"
puts [$libObj1 xml]

#puts [$libObj xml]
puts "isvalid test1: "
# puts [Rappture::library isvalid $libObj1]
puts "isvalid test2: "
# puts [Rappture::library isvalid library0]
