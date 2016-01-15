package require RapptureGUI

set commondir [file join [file dirname [info script]] .. common]
source [file join $commondir geovis_settings.tcl]

set width 400
set height 300
wm geometry . ${width}x${height}
update

Rappture::MapViewer::SetServerList $GEOVIS_SERVERS
set s [Rappture::MapViewer .g [list localhost:2015]]

pack .g -expand yes -fill both

# create an TMS layer
set dp1 [Rappture::GeoMapDataProviderTMS #auto \
    {http://readymap.org/readymap/tiles/1.0.0/7/}]
set l1 [Rappture::GeoMapLayerImage #auto $dp1 \
        -label  "ReadyMap Aerial" \
        -description "ReadyMap Satellite/Aerial Imagery" \
        -opacity 1.0 ]


# create a map object
set m [Rappture::Map #auto]

# add all layers to the map object
$m layer add -format blt_tree [$l1 export -format blt_tree]

# add a map to the vis client
$s scale $m
$s add $m
