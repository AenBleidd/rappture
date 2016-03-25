package require Rappture
package require RapptureGUI

Rappture::resources::load

set commondir [file join [file dirname [info script]] .. common]
source [file join $commondir geovis_settings.tcl]

set width 400
set height 300
wm geometry . ${width}x${height}
update

set mapviewer [Rappture::MapViewer .g]

pack .g -expand yes -fill both

# create a Gdal layer from a local geotiff
# here we use the local:// prefix to specify that the
# tif file needs to be transferred from the # client to the server.

array set gdalParams {
    url {local://world.tif}
}
array set layerParams {
    label "UTM GeoTIFF Layer"
    description "geotiff from gdal website"
    opacity 1.0
}

# create a map object
set map [Rappture::Map #auto]

# add all layers to the map object
$map addLayer image \
    test [array get layerParams] \
    gdal [array get gdalParams]

# add a map to the vis client
$mapviewer scale $map
$mapviewer add $map
