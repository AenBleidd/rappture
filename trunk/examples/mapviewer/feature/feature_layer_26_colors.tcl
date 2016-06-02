# This example demonstrates a problem that used to exist where
# sending too much data along with the command to the server
# would hit the command length limit. Data like stylesheets
# and selector information should now be sent over the data channel.

package require Tk
package require Rappture
package require RapptureGUI

set colors25 {
FF0000FF
FF2A00FF
FF5500FF
FF8000FF
FFAA00FF
FFD500FF
FFFF00FF
D5FF00FF
AAFF00FF
80FF00FF
55FF00FF
2BFF00FF
00FF00FF
00FF2AFF
00FF55FF
00FF80FF
00FFAAFF
00FFD4FF
00FFFFFF
00D4FFFF
00AAFFFF
0080FFFF
0055FFFF
002BFFFF
0000FFFF
}

set cutoffs25 {
-141000
5.65e+06
1.13e+07
1.70e+07
2.26e+07
2.83e+07
3.39e+07
3.96e+07
4.52e+07
5.09e+07
5.65e+07
6.22e+07
6.79e+07
7.35e+07
7.92e+07
8.48e+07
9.05e+07
9.61e+07
1.02e+08
1.07e+08
1.13e+08
1.19e+08
1.24e+08
1.30e+08
1.36e+08
1.41e+08
}

set colors26 {
FF0000FF
FF2900FF
FF5200FF
FF7A00FF
FFA300FF
FFCC00FF
FFF500FF
E0FF00FF
B8FF00FF
8FFF00FF
66FF00FF
3DFF00FF
14FF00FF
00FF14FF
00FF3DFF
00FF66FF
00FF8FFF
00FFB8FF
00FFE0FF
00F5FFFF
00CCFFFF
00A3FFFF
007AFFFF
0052FFFF
0029FFFF
0000FFFF
}

set cutoffs26 {
-141000
5.44e+06
1.09e+07
1.63e+07
2.17e+07
2.72e+07
3.26e+07
3.81e+07
4.35e+07
4.89e+07
5.44e+07
5.98e+07
6.52e+07
7.07e+07
7.61e+07
8.16e+07
8.70e+07
9.24e+07
9.79e+07
1.03e+08
1.09e+08
1.14e+08
1.20e+08
1.25e+08
1.30e+08
1.36e+08
1.41e+08
}


proc generateStyleSheet {colors} {

    set style_t {
      sID {
        fill: #COLOR;
        stroke: #000000ff;
        stroke-width: 3;
        altitude-clamping: terrain-drape;
      }
    }
    set stylesheet ""
    for {set i 0} {$i < [llength $colors]} {incr i} {
        set s $style_t
        regsub ID $s $i s
        regsub COLOR $s [lindex $colors $i] s
        append stylesheet $s
    }

    return $stylesheet
}

proc generateSelectors {cutoffs} {
    set selectors ""
    set numSelectors [llength $cutoffs]
    for {set i 0} {$i < $numSelectors-1} {incr i} {
       set lower [lindex $cutoffs $i]
       set upper [lindex $cutoffs [expr $i+1]]
       lappend selectors [list id $i style s$i query "POP2005 >= $lower and POP2005 < $upper"]
    }

    return $selectors
}



Rappture::resources::load

set commondir [file join [file dirname [info script]] .. common]
source [file join $commondir geovis_settings.tcl]

set width 400
set height 300
wm geometry . ${width}x${height}
update

set mapviewer [Rappture::MapViewer .g]

pack .g -expand yes -fill both


# Create a map object
set map [Rappture::Map #auto]

# Parameters for feature layer
array set ogrParams {
    url {file:///data/mygeohub/tools/landuse/afr_countries.shp}
}
array set countries {
    label   "Countries"
    opacity 1.0
}

set stylesheet [generateStyleSheet $colors25]
set selectors [generateSelectors $cutoffs25]

# Configure layers
$map addLayer feature \
    pop25 [array get countries] \
    ogr [array get ogrParams] \
    $stylesheet "" $selectors


# Add map to viewer
$mapviewer add $map
$mapviewer scale $map


after 5000 {
    $map deleteLayer "pop25"

    set stylesheet [generateStyleSheet $colors26]
    set selectors [generateSelectors $cutoffs26]

    # Configure layers
    $map addLayer feature \
        pop26 [array get countries] \
        ogr [array get ogrParams] \
        $stylesheet "" $selectors

    $mapviewer refresh
    puts "updating layer"
}
