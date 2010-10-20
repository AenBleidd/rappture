#!/usr/bin/env sh
#\
exec wish "$0" $*

package require RapptureGUI

#wm geometry . 600x300

set w 0
set t 0
set min 0
set max 7800

frame .f
pack .f -expand yes -fill both

Rappture::Flowdial .f.d1 \
    -length 10 \
    -valuewidth 0 \
    -valuepadding 0 \
    -padding 6 \
    -linecolor "" \
    -activelinecolor "" \
    -min $min \
    -max $max \
    -variable w \
    -knobimage [Rappture::icon knob2] \
    -knobposition center@middle

Rappture::Videodial .f.d3 \
    -length 10 \
    -valuewidth 0 \
    -valuepadding 0 \
    -padding 6 \
    -linecolor "" \
    -activelinecolor "red" \
    -min $min\
    -max $max \
    -minortick 20 \
    -majortick 100 \
    -knobimage "" \
    -knobposition center@bottom

#pack .f.d1 -expand yes -fill both
pack .f.d3 -expand yes -fill both

after 200 {
    update idletasks
    .f.d3 mark start 5
    .f.d3 mark end 76
    .f.d3 mark particle0 17
    .f.d3 mark particle1 10
    .f.d3 mark particle2 4
    .f.d3 mark particle3 54
    .f.d3 mark particle2 33
    .f.d3 mark particle1000 1001
    .f.d3 mark particle1100 1101
    .f.d3 mark particle1200 1201
    .f.d3 mark particle1300 1301
    .f.d3 mark particle1400 1401
    .f.d3 mark particle1500 1501
    .f.d3 mark particle1600 1601
    .f.d3 mark particle1700 1701
    .f.d3 mark particle1800 1801
    .f.d3 mark particle1900 1901
    .f.d3 mark particle2000 2001
    .f.d3 mark arrow 17
#    .f.d3 bball
}


set t 0
# after 500 play
proc play {} {
    global t min max
    if {($t+1) <= $max} {
        incr t
        .f.d3 current $t
        after 10 play
    }
}
