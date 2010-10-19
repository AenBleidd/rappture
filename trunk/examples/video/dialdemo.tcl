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
    -knobimage "" \
    -knobposition ne@bottom

#pack .f.d1 -expand yes -fill both
pack .f.d3 -expand yes -fill both

after 200 {
    update idletasks
#    .f.d3 mark start 5
#    .f.d3 mark end 10
#    .f.d3 mark particle0 17
#    .f.d3 mark particle1 10
#    .f.d3 mark particle2 4
#    .f.d3 mark particle3 54
#    .f.d3 mark particle2 33
#    .f.d3 mark arrow 17
    .f.d3 bball
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
