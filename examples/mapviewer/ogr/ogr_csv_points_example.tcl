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

# create an feature point layer from a csv file using the OGR driver
# the longitude and latitude fields must be named "longitude" and
# "latitude" respectively
#
# we add the local:// prefix to tell the map viewer widget that
# the file exists locally and needs to be transferred to the
# geovis server.
# set path "local://[file join [file dirname [info script]] coord.csv]"
set path "local://[file join [file dirname [info script]] station_clean.csv]"
set dp1 [Rappture::GeoMapDataProviderOGR #auto point $path]
set l1 [Rappture::GeoMapLayer #auto $dp1 \
        -label  "CSV point data" \
        -description "using OGR data provider to load CSV point data" \
        ]

# create a map object
set m [Rappture::Map #auto]

# add all layers to the map object
$m layer add -format blt_tree [$l1 export -format blt_tree]

# add a map to the vis client
$s scale $m
$s add $m
