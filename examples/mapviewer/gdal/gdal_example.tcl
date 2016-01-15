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

# create a Gdal layer from a local geotiff
# here we use the local:// prefix to specify that the
# tif file needs to be transferred from the # client to the server.

set path "local://[file join [file dirname [info script]] world.tif]"
set dp3 [Rappture::GeoMapDataProviderGdal #auto $path]
set l3 [Rappture::GeoMapLayerImage #auto $dp3 \
        -label  "UTM GeoTIFF Layer" \
        -description "geotiff from gdal website" \
        -opacity 1.0 ]

# create a map object
set m [Rappture::Map #auto]

# add all layers to the map object
$m layer add -format blt_tree [$l3 export -format blt_tree]

# add a map to the vis client
$s scale $m
$s add $m
