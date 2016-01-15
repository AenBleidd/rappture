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

# create an feature point layer from a csv file using the OGR driver
# the longitude and latitude fields must be named "longitude" and
# "latitude" respectively
#
# we add the local:// prefix to tell the map viewer widget that
# the file exists locally and needs to be transferred to the
# geovis server.
# set path "local://[file join [file dirname [info script]] coord.csv]"
set path "local://[file join [file dirname [info script]] station_clean.csv]"
set dp2 [Rappture::GeoMapDataProviderOGR #auto icon $path]
set l2 [Rappture::GeoMapLayer #auto $dp2 \
        -label  "CSV data" \
        -description "using OGR data provider to load data from a CSV file" \
        ]

# create a map object
set m [Rappture::Map #auto]

# add all layers to the map object
$m layer add -format blt_tree [$l1 export -format blt_tree]
$m layer add -format blt_tree [$l2 export -format blt_tree]

# add a map to the vis client
$s scale $m
$s add $m
