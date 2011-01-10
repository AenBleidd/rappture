#!/usr/bin/env sh
#\
exec wish "$0" $*

package require RapptureGUI

set w 0
set t 0
set min 0
#set max 7800
set max 100

frame .f
pack .f -expand yes -fill both

Rappture::Videodial2 .f.d \
    -padding 1 \
    -min $min\
    -max $max \
    -minortick 1 \
    -majortick 5

pack .f.d -expand yes -fill both

after 200 {
    update idletasks
#    .f.d mark add loopstart 5
#    .f.d mark add loopend 10
    .f.d mark add particle0 17
    .f.d mark add particle1 10
    .f.d mark add particle2 4
    .f.d mark add particle3 45
    .f.d mark add particle2 33
    .f.d mark add particle1200 20
    .f.d mark add particle1300 30
    .f.d mark add particle1400 40
    .f.d mark add particle1500 50
    .f.d mark add particle1600 60
    .f.d mark add particle1700 70
    .f.d mark add particle1800 80
    .f.d mark add particle1900 90
#    .f.d mark add particle1000 1000
#    .f.d mark add particle1100 1100
#    .f.d mark add particle1200 1200
#    .f.d mark add particle1300 1300
#    .f.d mark add particle1400 1400
#    .f.d mark add particle1500 1500
#    .f.d mark add particle1600 1600
#    .f.d mark add particle1700 1700
#    .f.d mark add particle1800 1800
#    .f.d mark add particle1900 1900
#    .f.d mark add particle2000 2000
    .f.d mark add arrow 17
#    .f.d bball
}


set t 6
#after idle play
proc play {} {
    global t min max
    if {($t+1) <= $max} {
        incr t
        .f.d current $t
        after 100 play
    }
}
