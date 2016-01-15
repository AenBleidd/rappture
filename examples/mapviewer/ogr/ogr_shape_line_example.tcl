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

# create an feature line layer from a shape file using the OGR driver
# we add the local:// prefix to tell the map viewer widget that
# the file exists locally and needs to be transferred to the
# geovis server. Because this is a shape file that could require
# many other external files, all files that start with "afr_countries"
# will also be transferred to the geovis server.
set path "local://[file join [file dirname [info script]] afr_countries.shp]"
set dp1 [Rappture::GeoMapDataProviderOGR #auto line $path]
set l1 [Rappture::GeoMapLayer #auto $dp1 \
        -label  "African Countries Shape File (Line)" \
        -description "Shape file with line data for African country borders" \
        ]

# create a map object
set m [Rappture::Map #auto]

# add all layers to the map object
$m layer add -format blt_tree [$l1 export -format blt_tree]

# add a map to the vis client
$s scale $m
$s add $m
