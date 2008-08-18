#!/usr/bin/wish

package require Rappture

set lib [Rappture::library difftest1.xml]
set lib2 [Rappture::library difftest2.xml]

set i 0

foreach {changeType path inVal outVal} [$lib diff $lib2] {
    set label ""

    if {$changeType == "c"} {
        set label [$lib get $path.about.label]
    }

    puts "---- Change $i ----"
    puts "changeType = $changeType"
    puts "path = $path"
    puts "inVal = $inVal"
    puts "outVal = $outVal"
    puts "label = $label"
    incr i
}
