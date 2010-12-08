#!/usr/bin/env sh
#\
exec wish "$0" $*

package require RapptureGUI

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
    -minortick 1 \
    -majortick 5 \
    -knobimage "" \
    -knobposition ne@bottom

#pack .f.d1 -expand yes -fill both
pack .f.d3 -expand yes -fill both

after 200 {
    update idletasks
    .f.d3 mark add loopstart 5
    .f.d3 mark add loopend 10
    .f.d3 mark add particle0 17
    .f.d3 mark add particle1 10
    .f.d3 mark add particle2 4
    .f.d3 mark add particle3 54
    .f.d3 mark add particle2 33
    .f.d3 mark add particle1000 1000
    .f.d3 mark add particle1100 1100
    .f.d3 mark add particle1200 1200
    .f.d3 mark add particle1300 1300
    .f.d3 mark add particle1400 1400
    .f.d3 mark add particle1500 1500
    .f.d3 mark add particle1600 1600
    .f.d3 mark add particle1700 1700
    .f.d3 mark add particle1800 1800
    .f.d3 mark add particle1900 1900
    .f.d3 mark add particle2000 2000
    .f.d3 mark add arrow 17
#    .f.d3 bball
}


set t 6000
#after idle play
proc play {} {
    global t min max
    if {($t+1) < 7000} {
        incr t
        .f.d3 current $t
        after 8 play
    }
}
