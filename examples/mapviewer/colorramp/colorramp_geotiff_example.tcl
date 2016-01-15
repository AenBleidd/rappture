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


# create an XYZ world layer
set dp1 [Rappture::GeoMapDataProviderXYZ #auto \
    {http://otile[1234].mqcdn.com/tiles/1.0.0/map/{z}/{x}/{y}.jpg}]
set l1 [Rappture::GeoMapLayerImage #auto $dp1 \
        -label  "OSM Map" \
        -description "MapQuest OpenStreetMap Street base layer" \
        -opacity 1.0 ]


# create a colorramp layer
set colormap {
 0.00 0 0 1 1
 0.25 0 1 1 1
 0.50 0 1 0 1
 0.75 1 1 0 1
 1.00 1 0 0 1
}

# load a layer data from a file named outlandish.tif
# located on the geovis server.
#
# the file is also located locally, and we could use
# the local:// prefix to specify the directory path.
set dp2 [Rappture::GeoMapDataProviderColorramp #auto \
    {file:///usr/share/geovis/landuse/outlandish.tif} \
    -colormap $colormap]
set l2 [Rappture::GeoMapLayerImage #auto $dp2 \
        -label  "Land Use" \
        -description "Land Use data from Landparam tool" \
        -opacity 1.0 \
        -coverage true]

# create a map object
set m [Rappture::Map #auto]

# add all layers to the map object
$m layer add -format blt_tree [$l1 export -format blt_tree]
$m layer add -format blt_tree [$l2 export -format blt_tree]

# add a map to the vis client
$s scale $m
$s add $m
